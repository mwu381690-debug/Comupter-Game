/**
 * ============================================================
 * ismcts.h — Information Set Monte Carlo Tree Search for Dou Di Zhu
 * ============================================================
 *
 * 【理论来源】
 *   Whitehouse, Powley, Cowling (CIG 2011, aisb 2011):
 *     "Determinization in Monte-Carlo Tree Search for the card game Dou Di Zhu"
 *   IEEE Conference on Computational Intelligence and Games
 *
 * 【算法原理】
 *   ISMCTS 在信息集树(information set tree)而非游戏状态树(game state tree)上操作。
 *   每个节点代表一个信息集(玩家可观测到的状态),而非完整的游戏状态。
 *   每次迭代中:
 *     1. Determinization: 随机分配未见牌给对手,得到一个确定化世界
 *     2. Selection:   从根节点出发,用UCB1公式选择子节点,直到叶节点
 *     3. Expansion:   在叶节点添加一个新的合法动作子节点
 *     4. Simulation:  用快速策略(贪心/随机)模拟到终局
 *     5. Backprop:    将结果沿路径回传,更新各节点的胜率统计
 *
 *   UCB1公式: 选择最大化 wins/visits + C * sqrt(ln(parent_visits) / visits) 的子节点
 *   其中 C 是探索常数(论文推荐 C=1.0)
 *
 * 【为什么ISMCTS优于确定化MCTS?】
 *   确定化MCTS: 对每个确定化世界独立建树搜索,最后合并根节点统计
 *   问题: 策略融合(strategy fusion) — 算法"认为"可以在不同世界使用不同策略,
 *         但实际上玩家无法区分信息集内的不同状态,必须选择相同的动作。
 *   ISMCTS: 在单一树上累积所有确定化世界的信息,避免了策略融合问题。
 *           在 Mini Dou Di Zhu 中,ISMCTS 胜率 67.2% vs 确定化MCTS 62.7%。
 *
 * 【实现要点】
 *   - 移动分组(Move Grouping): 将主牌和带牌分开作为两级决策,降低分支因子
 *     例如: "三带一"被拆分为 "选择三个J" → "选择一个带牌"
 *   - 快速Rollout: 使用简化贪心策略而非完整搜索
 *   - 迭代次数: 100-500次(论文使用10,000次,我们折中以适应实时要求)
 */

#ifndef ISMCTS_H
#define ISMCTS_H

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <ctime>
using namespace std;

// Forward declarations
struct Ddz;
int card2level(int card);
int AnalyzeTypeCount(int iCards[]);
int AnalyzeMainPoint(int iCards[]);
extern vector<int> remainCards;
extern int myPosition;
extern int cardRemaining[3];

// ============================================================
// ISMCTS Node: represents an information set in the game tree
// ============================================================
struct ISMCTSNode {
    int player;                    // Which player acts at this node (0=Landlord,1=FarmerA,2=FarmerB)
    int visits;                    // Times this node has been visited
    float wins;                    // Number of wins from this node (from root player's perspective)
    vector<int> action;            // The action that led TO this node (empty for root)
    ISMCTSNode* parent;            // Parent node
    vector<ISMCTSNode*> children;  // Child nodes (one per legal action)
    vector<vector<int>> legalActions; // All legal actions from this information set
    bool actionsGenerated;         // Have we generated legal actions yet?

    ISMCTSNode(int p, ISMCTSNode* par = nullptr)
        : player(p), visits(0), wins(0), parent(par), actionsGenerated(false) {}

    ~ISMCTSNode() {
        for (auto* child : children) delete child;
    }

    // UCB1 selection: pick child with highest UCB1 score
    ISMCTSNode* selectChild(float explorationConstant = 1.0f) {
        ISMCTSNode* best = nullptr;
        float bestScore = -1e9f;
        float logParentVisits = logf((float)max(1, visits));

        for (auto* child : children) {
            if (child->visits == 0) {
                // Always try unvisited children first (infinite UCB)
                return child;
            }
            float exploit = child->wins / (float)child->visits;
            float explore = explorationConstant * sqrtf(logParentVisits / (float)child->visits);
            float score = exploit + explore;
            if (score > bestScore) {
                bestScore = score;
                best = child;
            }
        }
        return best;
    }
};

// ============================================================
// Global ISMCTS state
// ============================================================
// Move grouping: separate main combo choice from kicker choice
struct MoveGroup {
    vector<int> mainCards;   // The main combo cards (e.g., three Js)
    vector<int> kickers;     // The kicker cards (e.g., a single 4)
    int typeCount;           // Combo type code
};

// ============================================================
// Helper: generate a determinization (assign unseen cards to opponents)
// ============================================================
void ismcts_determinize(vector<int>& oppA, vector<int>& oppB, const int* myHand) {
    oppA.clear(); oppB.clear();

    // Mark my cards
    bool mine[54] = {false};
    for (int i = 0; myHand[i] >= 0; i++) mine[myHand[i]] = true;

    // Collect unseen cards
    vector<int> unseen;
    for (int c = 0; c < 54; c++) {
        if (!mine[c]) unseen.push_back(c);
    }

    // Shuffle
    for (int i = unseen.size() - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap(unseen[i], unseen[j]);
    }

    // Distribute to opponents based on remaining card counts
    int oppA_count = cardRemaining[(myPosition + 1) % 3];
    int oppB_count = cardRemaining[(myPosition + 2) % 3];

    for (int i = 0; i < oppA_count && i < (int)unseen.size(); i++)
        oppA.push_back(unseen[i]);
    for (int i = oppA_count; i < oppA_count + oppB_count && i < (int)unseen.size(); i++)
        oppB.push_back(unseen[i]);
}

// ============================================================
// Fast rollout policy: simplified greedy playout
// ============================================================
// Generate legal actions for a given hand (simplified — only singles, pairs, triples, bombs)
void ismcts_getLegalActions(vector<int>& hand, int lastType, int lastLevel,
                             vector<vector<int>>& actions, bool freePlay) {
    actions.clear();
    if (hand.empty()) return;

    int counts[15] = {0};
    for (int c : hand) counts[card2level(c)]++;

    int typeBase = lastType / 100;

    if (freePlay || lastType == 0) {
        // Free to play anything — generate all simple combos
        // Singles
        for (int c : hand) actions.push_back({c});
        // Pairs
        for (int lv = 0; lv < 13; lv++) {
            if (counts[lv] >= 2) {
                vector<int> pair;
                int found = 0;
                for (int c : hand) {
                    if (card2level(c) == lv && found < 2) {
                        pair.push_back(c); found++;
                    }
                }
                if (pair.size() == 2) actions.push_back(pair);
            }
        }
        // Triples
        for (int lv = 0; lv < 13; lv++) {
            if (counts[lv] >= 3) {
                vector<int> triple;
                int found = 0;
                for (int c : hand) {
                    if (card2level(c) == lv && found < 3) {
                        triple.push_back(c); found++;
                    }
                }
                if (triple.size() == 3) actions.push_back(triple);
            }
        }
        // Bombs
        for (int lv = 0; lv < 13; lv++) {
            if (counts[lv] >= 4) {
                vector<int> bomb;
                int found = 0;
                for (int c : hand) {
                    if (card2level(c) == lv && found < 4) {
                        bomb.push_back(c); found++;
                    }
                }
                if (bomb.size() == 4) actions.push_back(bomb);
            }
        }
        // Rocket
        if (counts[13] >= 1 && counts[14] >= 1) {
            vector<int> rocket;
            for (int c : hand) {
                if (card2level(c) == 13 || card2level(c) == 14)
                    rocket.push_back(c);
            }
            if (rocket.size() == 2) actions.push_back(rocket);
        }
    } else {
        // Must beat the current play
        if (typeBase == 3) { // SINGLE
            for (int lv = lastLevel + 1; lv <= 14; lv++) {
                if (counts[lv] >= 1) {
                    for (int c : hand) {
                        if (card2level(c) == lv) {
                            actions.push_back({c}); break;
                        }
                    }
                }
            }
        } else if (typeBase == 4) { // PAIR
            for (int lv = lastLevel + 1; lv < 13; lv++) {
                if (counts[lv] >= 2) {
                    vector<int> pair; int found = 0;
                    for (int c : hand) {
                        if (card2level(c) == lv && found < 2) {
                            pair.push_back(c); found++;
                        }
                    }
                    if (pair.size() == 2) actions.push_back(pair);
                }
            }
        } else if (typeBase == 5 || typeBase == 6) { // TRIPLET/TRIPLET1/TRIPLET2
            for (int lv = lastLevel + 1; lv < 13; lv++) {
                if (counts[lv] >= 3) {
                    vector<int> triple; int found = 0;
                    for (int c : hand) {
                        if (card2level(c) == lv && found < 3) {
                            triple.push_back(c); found++;
                        }
                    }
                    if (triple.size() == 3) actions.push_back(triple);
                }
            }
        }
        // Bombs can always beat non-bomb plays
        for (int lv = 0; lv < 13; lv++) {
            if (counts[lv] >= 4) {
                vector<int> bomb; int found = 0;
                for (int c : hand) {
                    if (card2level(c) == lv && found < 4) {
                        bomb.push_back(c); found++;
                    }
                }
                if (bomb.size() == 4) actions.push_back(bomb);
            }
        }
        // Rocket beats everything
        if (counts[13] >= 1 && counts[14] >= 1) {
            vector<int> rocket;
            for (int c : hand) {
                if (card2level(c) == 13 || card2level(c) == 14)
                    rocket.push_back(c);
            }
            if (rocket.size() == 2) actions.push_back(rocket);
        }
    }
}

// Remove cards from hand
void ismcts_removeCards(vector<int>& hand, const vector<int>& played) {
    for (int c : played) {
        auto it = find(hand.begin(), hand.end(), c);
        if (it != hand.end()) hand.erase(it);
    }
}

// Fast rollout: simulate from current state to game end, return winner
int ismcts_rollout(vector<int> hands[3], int startPlayer,
                    int lastType, int lastLevel, int lastPassCount, int maxTurns = 100) {
    int currentPlayer = startPlayer;
    for (int turn = 0; turn < maxTurns; turn++) {
        // Check for winner
        for (int p = 0; p < 3; p++) {
            if (hands[p].empty()) return p;
        }

        vector<int>& curHand = hands[currentPlayer];
        bool freePlay = (lastPassCount >= 2 || lastType == 0);

        vector<vector<int>> legalActions;
        ismcts_getLegalActions(curHand, lastType, lastLevel, legalActions, freePlay);

        if (legalActions.empty()) {
            // PASS
            lastPassCount++;
            if (lastPassCount >= 2) {
                lastType = 0; lastLevel = -1; lastPassCount = 0;
            }
        } else {
            // Play a random legal action (simple rollout policy)
            int idx = rand() % legalActions.size();
            vector<int>& action = legalActions[idx];

            // Compute the type of the action
            int actionArr[21]; int ai = 0;
            for (int c : action) actionArr[ai++] = c;
            actionArr[ai] = -1;
            lastType = AnalyzeTypeCount(actionArr);
            lastLevel = AnalyzeMainPoint(actionArr);
            lastPassCount = 0;

            // Remove played cards
            ismcts_removeCards(curHand, action);
        }

        currentPlayer = (currentPlayer + 1) % 3;
    }
    return -1; // Timeout
}

// ============================================================
// ISMCTS: Main search function
// ============================================================
// Returns the best action (as card indices) found by ISMCTS.
// The action includes -1 as sentinel at the end.
vector<int> ismcts_search(struct Ddz* pDdz, int numIterations = 200,
                           float explorationConstant = 1.0f) {
    // Build my hand vector
    vector<int> myHand;
    for (int i = 0; pDdz->iOnHand[i] >= 0; i++)
        myHand.push_back(pDdz->iOnHand[i]);

    // Generate legal actions using the existing platform HelpPla
    // (Already done by CalPla before calling this function)
    // We use pDdz->iPlaArr which has all legal actions

    // Create root node (current player = myPosition)
    ISMCTSNode* root = new ISMCTSNode(myPosition);

    // Populate root's legal actions from pDdz->iPlaArr
    for (int i = 0; i < pDdz->iPlaCount; i++) {
        vector<int> action;
        for (int j = 0; pDdz->iPlaArr[i][j] >= 0; j++)
            action.push_back(pDdz->iPlaArr[i][j]);
        root->legalActions.push_back(action);

        // Create child node for this action
        ISMCTSNode* child = new ISMCTSNode((myPosition + 1) % 3, root);
        child->action = action;
        root->children.push_back(child);
    }
    root->actionsGenerated = true;

    // ISMCTS main loop
    for (int iter = 0; iter < numIterations; iter++) {
        // ── 1. Determinization ──
        vector<int> oppA_hand, oppB_hand;
        ismcts_determinize(oppA_hand, oppB_hand, pDdz->iOnHand);

        // Map positions to hands
        // Position mapping: 0=Landlord, 1=FarmerA, 2=FarmerB
        vector<int>* posHands[3];
        if (myPosition == 0) {
            posHands[0] = &myHand;
            posHands[1] = &oppA_hand;
            posHands[2] = &oppB_hand;
        } else if (myPosition == 1) {
            posHands[0] = &oppA_hand;
            posHands[1] = &myHand;
            posHands[2] = &oppB_hand;
        } else {
            posHands[0] = &oppA_hand;
            posHands[1] = &oppB_hand;
            posHands[2] = &myHand;
        }

        // ── 2. Selection: traverse tree using UCB1 ──
        ISMCTSNode* node = root;
        int currentPlayer = myPosition;
        int lastType = 0, lastLevel = -1, lastPassCount = 0;

        // Working copies of hands for this determinization
        vector<int> workHands[3];
        workHands[0] = *posHands[0];
        workHands[1] = *posHands[1];
        workHands[2] = *posHands[2];

        // Selection phase
        while (node->actionsGenerated && !node->children.empty()) {
            node = node->selectChild(explorationConstant);
            if (!node) break;

            // Apply the action
            ismcts_removeCards(workHands[currentPlayer], node->action);

            int actionArr[21]; int ai = 0;
            for (int c : node->action) actionArr[ai++] = c;
            actionArr[ai] = -1;
            lastType = AnalyzeTypeCount(actionArr);
            lastLevel = AnalyzeMainPoint(actionArr);
            lastPassCount = 0;

            currentPlayer = (currentPlayer + 1) % 3;

            // Check if game ended
            if (workHands[currentPlayer].empty()) break;
        }

        // ── 3. Expansion ──
        if (node && !node->actionsGenerated && !workHands[currentPlayer].empty()) {
            bool freePlay = (lastPassCount >= 2 || lastType == 0);
            ismcts_getLegalActions(workHands[currentPlayer], lastType, lastLevel,
                                    node->legalActions, freePlay);

            // Include PASS as an option
            node->legalActions.push_back({-1}); // -1 represents PASS

            for (auto& action : node->legalActions) {
                ISMCTSNode* child = new ISMCTSNode((currentPlayer + 1) % 3, node);
                child->action = action;
                node->children.push_back(child);
            }
            node->actionsGenerated = true;

            // Pick first child for rollout
            if (!node->children.empty()) {
                node = node->children[0];
                if (node->action[0] != -1) { // Not a PASS
                    ismcts_removeCards(workHands[currentPlayer], node->action);
                    int actionArr[21]; int ai = 0;
                    for (int c : node->action) actionArr[ai++] = c;
                    actionArr[ai] = -1;
                    lastType = AnalyzeTypeCount(actionArr);
                    lastLevel = AnalyzeMainPoint(actionArr);
                    lastPassCount = 0;
                } else {
                    lastPassCount++;
                    if (lastPassCount >= 2) {
                        lastType = 0; lastLevel = -1; lastPassCount = 0;
                    }
                }
                currentPlayer = (currentPlayer + 1) % 3;
            }
        }

        // ── 4. Simulation (Rollout) ──
        int winner = ismcts_rollout(workHands, currentPlayer, lastType, lastLevel, lastPassCount);

        // ── 5. Backpropagation ──
        // Determine if root player won
        bool rootWins = false;
        if (myPosition == 0) rootWins = (winner == 0);
        else rootWins = (winner == 1 || winner == 2);

        // Backpropagate from node up to root
        ISMCTSNode* bpNode = node;
        while (bpNode) {
            bpNode->visits++;
            if (rootWins) bpNode->wins += 1.0f;
            bpNode = bpNode->parent;
        }
    }

    // ── Choose best action ──
    // Pick the child with the highest win rate (exploitation only at root)
    int bestIdx = -1;
    float bestRate = -1.0f;

    for (int i = 0; i < (int)root->children.size(); i++) {
        ISMCTSNode* child = root->children[i];
        if (child->visits > 0) {
            float rate = child->wins / (float)child->visits;
            if (rate > bestRate) {
                bestRate = rate;
                bestIdx = i;
            }
        }
    }

    // Build result
    vector<int> result;
    if (bestIdx >= 0 && bestIdx < (int)root->legalActions.size()) {
        result = root->legalActions[bestIdx];
        if (!result.empty() && result.back() == -1) result.pop_back(); // Remove PASS sentinel
    }

    // Clean up
    delete root;

    return result;
}

#endif // ISMCTS_H
