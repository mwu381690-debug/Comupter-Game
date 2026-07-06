/**
 * ============================================================
 * opponent_model.h — 贝叶斯对手手牌推理 & 蒙特卡洛出牌评估模块
 * ============================================================
 *
 * 【理论来源】
 *   1. DouZero+ (Zhao et al., IEEE Transactions on Games, 2024)
 *      - 对手建模: 多分类器预测下家手牌分布
 *      - Coach网络: 过滤不平衡开局加速训练
 *      - MC仿真叫地主网络
 *   2. DouZero (Zha et al., ICML 2021)
 *      - Deep Monte-Carlo (DMC): 用神经网络近似Q值,自博弈训练
 *      - 证明了MC方法在斗地主大动作空间中优于DQN/A3C
 *   3. Bayes'Bluff (Southey et al., UAI 2005)
 *      - 贝叶斯概率模型分离游戏动态不确定性与对手策略不确定性
 *      - Dirichlet先验对手建模，后验推理+最优响应
 *   4. ISMCTS for Dou Di Zhu (Whitehouse et al., CIG 2011)
 *      - 信息集蒙特卡洛树搜索：在信息集树而非状态树上操作
 *      - 确定化采样 + UCT, 解决策略融合(strategy fusion)和非局域性(non-locality)
 *   5. Understanding PIMC (Long et al., AAAI 2010)
 *      - PIMC成功条件: 叶节点相关性高、偏差大、消歧因子适中
 *      - 斗地主满足这些条件 → PIMC/确定化方法有效
 *
 * 【模块组成】
 *   模块一: 贝叶斯对手手牌推理
 *     - initOpponentModel()        游戏开始时初始化手牌分布
 *     - rebuildOpponentModel()     从历史出牌完整重建对手模型
 *     - getOpponentCardProb()      查询对手持有某等级牌的概率
 *     - isLikelyBiggest()          概率化判断牌型是否为"控手"(最大)
 *     - oppCardsSeen[][]           每个对手已出牌按等级统计
 *     - oppCardsMax[][]            每个对手最多可能持有的每级牌数
 *     - oppHistory[]               对手行为历史(PASS次数/类型等)
 *
 *   模块二: 蒙特卡洛出牌评估
 *     - determinize()              确定化: 随机分配未见牌给对手
 *     - simulateOpponentTurn()     模拟对手回合(简化贪心策略)
 *     - mcEvaluateAction()        对候选出牌跑N次MC模拟,返回胜率估计
 *     - quickHandEval()            轻量手牌评估(用于MC rollout内部)
 *
 * 【核心原理】
 *   ┌─────────────────────────────────────────────────┐
 *   │ 贝叶斯推理核心逻辑:                                │
 *   │ 1. 对手出牌 → 确定性地从手牌移除                    │
 *   │ 2. 对手PASS → 约束传播:                            │
 *   │    如果对手对level=L的SINGLE PASS了,              │
 *   │    那么对手很可能没有level>L的单牌                  │
 *   │    (除非他在战略性保留炸弹/火箭)                    │
 *   │ 3. 概率化控手判断:                                 │
 *   │    不再简单返回true/false,而是计算                  │
 *   │    P(对手能压制) = Σ P(对手有更大同型牌)           │
 *   │                  + P(对手有炸弹) + P(对手有火箭)    │
 *   └─────────────────────────────────────────────────┘
 *
 * 【MC评估流程】
 *   for 每个候选出牌方案:
 *     for N次采样:
 *       1. determinize(): 随机分配未见牌
 *       2. 模拟对战直到有人出完牌
 *       3. 记录输赢
 *     胜率 = 赢的次数 / N
 *   选胜率最高的方案
 */

#ifndef OPPONENT_MODEL_H
#define OPPONENT_MODEL_H

#include <vector>
#include <cstring>
#include <algorithm>
#include "DdzV200.h"                  // 游戏框架: Ddz结构体, 牌型枚举, card2level等
using namespace std;

// ============================================================
// 引用主文件中定义的全局变量
// ============================================================
extern vector<int> othersCards;      // 所有未见牌按等级统计 (15个等级)
extern vector<int> remainCards;      // 54张牌的剩余标记数组
extern int myPosition;               // 我方身份: 0=地主, 1=农民A, 2=农民B
extern int cardRemaining[3];         // 三个玩家各剩余几张牌

// ============================================================
// 模块一: 贝叶斯对手手牌推理
// ============================================================

/**
 * 对手手牌追踪数组
 * oppCardsSeen[opp][level]: 我们已经看到对手opp打出的level等级的牌数量
 * oppCardsMax[opp][level]:  对手opp最多可能持有的level等级的牌数量上限
 *
 * opp索引: 0=下家对手, 1=上家对手 (相对我方位置)
 * 例如: 我是地主(0), 则 opp[0]=农民A(1), opp[1]=农民B(2)
 *       我是农民A(1), 则 opp[0]=地主(0), opp[1]=农民B(2)
 */
int oppCardsSeen[2][15];     // 已看到每个对手打出的牌,按等级统计
int oppCardsMax[2][15];      // 每个对手每等级最多持有的牌数(上限约束)

/**
 * 对手行为历史
 * 用于从对手的出牌/PASS行为中推理其手牌强度
 */
struct OpponentHistory {
    int passCount;           // 累计PASS次数
    int lastPassType;        // 最后一次PASS时场上的牌型编码 (0=未曾PASS)
    int lastPassLevel;       // 最后一次PASS时场上的牌型主级别
    int aggressivePlays;     // 激进出牌次数 (炸弹、大牌压制)
    int conservativePlays;   // 保守出牌次数 (小单牌、小对子)
};
OpponentHistory oppHistory[2];  // 两个对手各一份行为历史

/**
 * 查询对手持有某等级至少minCount张牌的平滑概率
 *
 * @param oppIdx   对手索引 (0或1)
 * @param lv       牌等级 (0=3, 1=4, ..., 12=2, 13=小王, 14=大王)
 * @param minCount 最少需要的张数
 * @return         概率估计 [0.0, 1.0]
 *
 * 算法:
 *   1. 如果上限约束都不满足,直接返回0
 *   2. 基线上认为均匀分布,概率=1.0
 *   3. 如果对手曾经对同类型低级别牌PASS过,
 *      说明他可能没有更高级别的该类型牌,乘以折扣因子0.5
 */
float getOpponentCardProb(int oppIdx, int lv, int minCount) {
    // 硬约束: 上限不够,不可能持有
    if (oppCardsMax[oppIdx][lv] < minCount) return 0.0f;

    float prob = 1.0f;

    // 软约束: 从PASS历史推理
    // 如果对手曾经对同类型低级别牌PASS, 则持有更高级别牌的概率降低
    if (oppHistory[oppIdx].lastPassType > 0) {
        int passType = oppHistory[oppIdx].lastPassType / 100;   // 提取牌型类别
        int passLevel = oppHistory[oppIdx].lastPassLevel;        // 提取等级
        // 对于单牌/对子/三带这类简单牌型, PASS传递了强信号
        if ((passType == 3 || passType == 4 || passType == 5)
            && lv > passLevel) {
            prob *= 0.5f;  // 折半: 对手有50%概率在保留实力
        }
    }

    return prob;
}

/**
 * 游戏开始时初始化对手手牌模型
 *
 * 将所有未见牌按均匀分布分配给两个对手,
 * 建立初始的oppCardsMax[][]上限数组。
 *
 * @param pDdz  游戏状态指针
 */
void initOpponentModel(struct Ddz * pDdz) {
    // 清零所有追踪数组
    memset(oppCardsSeen, 0, sizeof(oppCardsSeen));
    memset(oppCardsMax, 0, sizeof(oppCardsMax));
    memset(oppHistory, 0, sizeof(oppHistory));

    // 标记已知牌: 我方手牌 + 底牌
    bool myCards[54] = {false};
    for (int i = 0; pDdz->iOnHand[i] >= 0; i++)
        myCards[pDdz->iOnHand[i]] = true;   // 我的手牌
    for (int i = 0; i < 3; i++)
        myCards[pDdz->iLef[i]] = true;       // 三张底牌

    // 统计未见牌按等级分布
    int unseenPerLevel[15] = {0};
    for (int c = 0; c < 54; c++) {
        if (!myCards[c])
            unseenPerLevel[card2level(c)]++;
    }

    // 初始化: 两个对手的上限都等于未见牌总数
    // (因为每张未见牌都可能在其中任意一个对手手中)
    for (int lv = 0; lv < 15; lv++) {
        for (int opp = 0; opp < 2; opp++) {
            oppCardsMax[opp][lv] = unseenPerLevel[lv];
        }
    }
}

/**
 * 从完整游戏历史重建对手模型
 *
 * 遍历桌上所有历史出牌记录(iOnTable[0..iOTmax]),
 * 区分我方/下家对手/上家对手的出牌和PASS,
 * 重建对手手牌追踪和行为历史。
 *
 * @param pDdz  游戏状态指针
 *
 * 算法:
 *   for 每一轮出牌历史:
 *     判断出牌者身份 → 映射到oppIdx
 *     if 出牌==PASS(-1):
 *       找到被PASS的那一手牌 → 记录对手PASS行为
 *     else:
 *       记录对手打出了哪些牌 → 更新oppCardsSeen
 */
void rebuildOpponentModel(struct Ddz * pDdz) {
    // 先重置为初始状态
    initOpponentModel(pDdz);

    // 遍历所有历史回合
    for (int turn = 0; pDdz->iOnTable[turn][0] != -2; turn++) {
        int player = turn % 3;  // 该回合出牌者(0=A, 1=B, 2=C)

        // ──── 将玩家编号映射为对手索引 ────
        int oppIdx = -1;
        if (myPosition == 0) {        // 我是地主
            if (player == 1) oppIdx = 0;   // 农民A → opp[0]
            if (player == 2) oppIdx = 1;   // 农民B → opp[1]
        } else if (myPosition == 1) { // 我是农民A
            if (player == 0) oppIdx = 0;   // 地主 → opp[0]
            if (player == 2) oppIdx = 1;   // 农民B → opp[1]
        } else {                      // 我是农民B
            if (player == 0) oppIdx = 0;   // 地主 → opp[0]
            if (player == 1) oppIdx = 1;   // 农民A → opp[1]
        }

        if (oppIdx < 0) continue;  // 跳过自己

        // ──── 处理PASS ────
        if (pDdz->iOnTable[turn][0] == -1) {
            // 对手PASS了。找到他是在对哪一手牌PASS
            int prevTurn = turn - 1;
            while (prevTurn >= 0 && pDdz->iOnTable[prevTurn][0] == -1)
                prevTurn--;  // 跳过连续的PASS

            if (prevTurn >= 0 && pDdz->iOnTable[prevTurn][0] >= 0) {
                // 找到了被PASS的那手牌,解析其牌型和等级
                int prevCards[21];
                int idx = 0;
                for (; pDdz->iOnTable[prevTurn][idx] >= 0; idx++)
                    prevCards[idx] = pDdz->iOnTable[prevTurn][idx];
                prevCards[idx] = -1;  // 哨兵

                int typeCount = AnalyzeTypeCount(prevCards);  // 牌型编码
                int mainPoint = AnalyzeMainPoint(prevCards);  // 主级别

                // 记录这次PASS行为
                oppHistory[oppIdx].passCount++;
                oppHistory[oppIdx].lastPassType = typeCount;
                oppHistory[oppIdx].lastPassLevel = mainPoint;
            }
        }
        // ──── 处理出牌 ────
        else {
            // 记录对手打出的每一张牌
            for (int i = 0; pDdz->iOnTable[turn][i] >= 0; i++) {
                int lv = card2level(pDdz->iOnTable[turn][i]);
                oppCardsSeen[oppIdx][lv]++;  // 该对手已打出lv等级的牌+1
            }
            // 出牌后清除上次PASS标记
            oppHistory[oppIdx].lastPassType = 0;
        }
    }
}

/**
 * 概率化控手判断: 这一手牌是否"很可能"是当前类型中最大的?
 *
 * @param lv            牌型的主级别
 * @param requiredCount 该牌型每级需要的牌数(单牌=1, 对子=2, 三带=3, 炸弹=4)
 * @param comboType     牌型编码(如301=SINGLE, 402=PAIR, 204=BOMB)
 * @return              true=很可能是最大的, false=可能被压制
 *
 * 【设计理念】
 *   原始代码用简单的布尔判断: othersCards[l] > 0 → biggest=false
 *   这过于粗糙。实际情况是:
 *     - 对手可能有更大的同型牌
 *     - 对手可能有炸弹/火箭
 *     - 但如果对手之前PASS过同类型低级别牌,则可能性降低
 *
 *   改进: 计算 P(被压制) = Σ P(对手有更大的同型牌)
 *                      + P(对手有炸弹) + P(对手有火箭)
 *   如果 P(被压制) < 0.5, 则认为是"控手牌"
 *
 *   额外考虑对手PASS历史的折扣效应:
 *     对手对单牌9 PASS → 对手持有单牌10~A的概率降低40%
 */
bool isLikelyBiggest(int lv, int requiredCount, int comboType) {
    float probCanBeBeaten = 0.0f;  // 被压制的累计概率
    int typeBase = comboType / 100; // 提取牌型类别(3=单,4=对,5=三,7=顺,...)

    // 简单牌型可以检查到14(大王), 顺子类只能到12(A,因为2不能参与顺子)
    int maxCheckLevel = (typeBase == 3 || typeBase == 4 || typeBase == 5) ? 14 : 12;

    // ──── 逐级计算被更大同型牌压制的概率 ────
    for (int l = lv + 1; l <= maxCheckLevel; l++) {
        if (othersCards[l] >= requiredCount) {
            // 基础概率: 有更大的同型牌存在 → 大概率会被压制
            probCanBeBeaten += 0.8f;

            // PASS折扣: 如果对手曾经对同类型低级别牌PASS过,
            // 说明他们可能没有更高级别的该类型牌
            float passDiscount = 1.0f;
            for (int opp = 0; opp < 2; opp++) {
                if (oppHistory[opp].lastPassType > 0 &&
                    oppHistory[opp].lastPassType / 100 == typeBase &&
                    oppHistory[opp].lastPassLevel < l) {
                    // 该对手之前PASS了同类型更低级别的牌
                    // → 他持有更高级别牌的概率应打折
                    passDiscount *= 0.6f;
                }
            }
            // 调整: 从原来的0.8替换为打折后的值
            probCanBeBeaten += (0.8f * passDiscount - 0.8f);
        }
    }

    // ──── 考虑炸弹和火箭的威胁 ────
    // 对于单牌/对子/三带级别,炸弹和火箭是万能压制
    if (requiredCount <= 3) {
        // 检查是否有火箭(双王)存在
        bool hasRocket = (othersCards[13] > 0 && othersCards[14] > 0);
        // 检查是否有炸弹存在
        bool hasBomb = false;
        for (int l = 0; l < 13; l++) {
            if (othersCards[l] >= 4) { hasBomb = true; break; }
        }
        if (hasRocket) probCanBeBeaten += 1.0f;   // 火箭几乎必然会被用来压制
        if (hasBomb)  probCanBeBeaten += 0.5f;     // 炸弹有中等概率被用来压制
    }

    // 阈值0.5: 被压制概率不到一半 → 认为是控手牌
    return probCanBeBeaten < 0.5f;
}


// ============================================================
// 模块二: 蒙特卡洛出牌评估 (MC Rollout)
// ============================================================

// MC模块需要引用的外部变量
extern float roundValWithDecay;  // 控手牌基础价值(=7)
extern float baseCardsWeight;    // 基础牌权重(=1.0)
extern float SeqDecayRate;       // 顺子衰减率(=1.0)
extern float RocketVal;          // 火箭价值(=20)
extern int DirConvertPara;       // 方位转换参数
extern int lastPlayer;           // 上一个有效出牌者
extern bool iCanWin;             // 是否能赢标记

/**
 * 确定化采样: 将未见牌随机分配给两个对手
 *
 * 【背景知识 - 确定化(Determinization)】
 *   不完美信息博弈中,将游戏"确定化"为多个完美信息实例,
 *   每个实例中对手手牌是固定的,然后对每个实例独立搜索,
 *   最后合并结果。这是PIMC(完美信息蒙特卡洛)的核心思想。
 *
 *   虽然理论上确定化有策略融合(strategy fusion)和非局域性(non-locality)
 *   两个缺陷(Frank & Basin, 1998),但Long et al.(2010)证明了
 *   斗地主类游戏满足PIMC有效的条件(高叶节点相关性+适中消歧因子),
 *   因此确定化方法是适用的。
 *
 * @param oppA_hand  输出: 对手A的手牌
 * @param oppB_hand  输出: 对手B的手牌
 */
void determinize(vector<int>& oppA_hand, vector<int>& oppB_hand) {
    oppA_hand.clear();
    oppB_hand.clear();

    // 收集所有未见牌(既不在我手,也没被打出)
    vector<int> unseen;
    for (int c = 0; c < 54; c++) {
        if (remainCards[c]) {  // remainCards由setOthersCards()维护
            unseen.push_back(c);
        }
    }

    // 随机打乱未见牌顺序
    random_shuffle(unseen.begin(), unseen.end());

    // 按对手剩余牌数分配
    int oppA_count = cardRemaining[(myPosition + 1) % 3];  // 下家剩余牌数
    int oppB_count = cardRemaining[(myPosition + 2) % 3];  // 上家剩余牌数

    // 按序分配给两个对手
    for (int i = 0; i < oppA_count && i < (int)unseen.size(); i++)
        oppA_hand.push_back(unseen[i]);
    for (int i = oppA_count; i < oppA_count + oppB_count && i < (int)unseen.size(); i++)
        oppB_hand.push_back(unseen[i]);
}

/**
 * 轻量级手牌评估 (用于MC rollout内部,比完整CalCardsValue快得多)
 *
 * 启发式评分:
 *   - 牌越少越好
 *   - 单牌(孤张)是累赘,权重-15
 *   - 对子中性,权重-5
 *   - 三带是好牌,权重+8
 *   - 炸弹是王牌,权重+20
 *   - 大牌比小牌好,每级+0.5
 *
 * @param hand  手牌
 * @return      估值分数(越高越好)
 */
float quickHandEval(vector<int>& hand) {
    if (hand.empty()) return 1000.0f;  // 空手=获胜,极高分数

    // 按等级统计
    int counts[15] = {0};
    for (int c : hand) counts[card2level(c)]++;

    int totalCards = hand.size();
    int singletons = 0;  // 单牌数
    int pairs = 0;       // 对子数
    int triples = 0;     // 三带数
    int bombs = 0;       // 炸弹数

    for (int lv = 0; lv < 15; lv++) {
        if (counts[lv] == 1) singletons++;
        else if (counts[lv] == 2) pairs++;
        else if (counts[lv] == 3) triples++;
        else if (counts[lv] == 4) bombs++;
    }

    // 启发式估值公式
    float value = -totalCards * 2.0f;   // 基础: 每张牌-2分
    value -= singletons * 15.0f;        // 单牌很糟糕,每张-15
    value -= pairs * 5.0f;              // 对子中性,每对-5
    value += triples * 8.0f;            // 三带不错,每个+8
    value += bombs * 20.0f;             // 炸弹极好,每个+20

    // 大牌价值加成
    for (int c : hand) {
        value += (float)card2level(c) * 0.5f;
    }

    return value;
}

/**
 * 模拟一个对手的回合 (MC rollout中调用)
 *
 * 使用简化贪心策略:
 *   1. 如果可以自由出牌(上轮被PASS×2或新一轮) → 出最小的单牌
 *   2. 否则尝试用同类型更大牌压制
 *   3. 压制不了就检查炸弹/火箭
 *   4. 都没有就PASS
 *
 * @param oppIdx          对手在oppCards中的索引
 * @param oppHand         对手当前手牌(会被修改)
 * @param lastTypeCount   当前场上的牌型编码(会被更新)
 * @param lastMainPoint   当前场上的牌型主级别(会被更新)
 * @param lastPassCount   连续PASS计数(会被更新)
 * @return                -1=游戏继续, >=0=该玩家获胜(出完了所有牌)
 */
int simulateOpponentTurn(int oppIdx, vector<int>& oppHand, int& lastTypeCount,
                          int& lastMainPoint, int& lastPassCount) {
    // 该对手已经出完牌 → 获胜
    if (oppHand.empty()) return oppIdx;

    sort(oppHand.begin(), oppHand.end());

    // ──── 情况1: 可以自由出牌(新一轮) ────
    // 连续两次PASS后牌权重置,或者这是新的一轮(lastTypeCount==0)
    if (lastPassCount >= 2 || lastTypeCount == 0) {
        // 策略: 出最小的单牌(贪心)
        int played = oppHand[0];
        oppHand.erase(oppHand.begin());
        lastTypeCount = 301;               // 301 = SINGLE(3)*100 + 1张
        lastMainPoint = card2level(played);
        lastPassCount = 0;
        return -1;
    }

    // ──── 情况2: 尝试压制当前场上的牌 ────
    int typeBase = lastTypeCount / 100;     // 牌型类别
    int level = lastMainPoint;              // 需要压制的等级
    bool found = false;

    // 统计手牌按等级分布
    int counts[15] = {0};
    for (int c : oppHand) counts[card2level(c)]++;

    // ──── 尝试同类型压制: 单牌 ────
    if (typeBase == 3) {
        // 从高一级开始找,找到第一个可用的单牌就出
        for (int lv = level + 1; lv <= 14; lv++) {
            if (counts[lv] >= 1) {
                for (auto it = oppHand.begin(); it != oppHand.end(); ++it) {
                    if (card2level(*it) == lv) {
                        oppHand.erase(it);
                        break;
                    }
                }
                lastTypeCount = 301;
                lastMainPoint = lv;
                lastPassCount = 0;
                found = true;
                break;
            }
        }
    }
    // ──── 尝试同类型压制: 对子 ────
    else if (typeBase == 4) {
        for (int lv = level + 1; lv < 13; lv++) {  // 2不能组成对子中的顺子限制,保守到12
            if (counts[lv] >= 2) {
                int removed = 0;
                for (auto it = oppHand.begin(); it != oppHand.end() && removed < 2; ) {
                    if (card2level(*it) == lv) {
                        it = oppHand.erase(it);
                        removed++;
                    } else ++it;
                }
                lastTypeCount = 402;   // 402 = PAIR(4)*100 + 2张
                lastMainPoint = lv;
                lastPassCount = 0;
                found = true;
                break;
            }
        }
    }

    // ──── 同类型压制不了,尝试炸弹 ────
    if (!found) {
        for (int lv = 0; lv < 13; lv++) {
            if (counts[lv] >= 4) {
                int removed = 0;
                for (auto it = oppHand.begin(); it != oppHand.end() && removed < 4; ) {
                    if (card2level(*it) == lv) {
                        it = oppHand.erase(it);
                        removed++;
                    } else ++it;
                }
                lastTypeCount = 204;   // 204 = BOMB(2)*100 + 4张
                lastMainPoint = lv;
                lastPassCount = 0;
                found = true;
                break;
            }
        }
    }

    // ──── 尝试火箭(双王) ────
    if (!found && counts[13] >= 1 && counts[14] >= 1) {
        for (auto it = oppHand.begin(); it != oppHand.end(); ) {
            if (card2level(*it) == 13 || card2level(*it) == 14) {
                it = oppHand.erase(it);
            } else ++it;
        }
        lastTypeCount = 102;   // 102 = ROCKET(1)*100 + 2张
        lastMainPoint = 14;
        lastPassCount = 0;
        found = true;
    }

    // ──── 什么都没法出 → PASS ────
    if (!found) {
        lastPassCount++;
        // 连续两次PASS后重置牌权
        if (lastPassCount >= 2) {
            lastTypeCount = 0;
            lastMainPoint = -1;
            lastPassCount = 0;
        }
    }

    return -1;  // 游戏继续
}

/**
 * 蒙特卡洛评估一个候选出牌方案
 *
 * 【算法流程 - Deep Monte-Carlo (DouZero, ICML 2021)】
 *   for sample in 1..numSamples:
 *     1. 确定化(determinize): 随机分配未见牌给对手
 *     2. 从当前状态开始模拟对战:
 *        - 我已经出了候选牌,场上牌型=候选牌型
 *        - 按轮次模拟三方出牌
 *        - 我方用简化贪心策略
 *        - 对手用simulateOpponentTurn()
 *     3. 直到有人出完牌 → 记录输赢
 *   返回 胜率 = 赢的样本数 / 总样本数
 *
 * 【为什么MC方法在斗地主中优于DQN? (DouZero论文)】
 *   1. DQN的过估计偏差(bootstrap bias)在大动作空间中很严重
 *   2. 斗地主是稀疏奖励、长视野任务, DQN收敛慢
 *   3. MC方法直接近似真实回报,无偏差
 *   4. MC方法可以自然利用动作特征(action features)泛化到未见动作
 *
 * @param pDdz        游戏状态指针
 * @param action      候选出牌方案(含-1哨兵)
 * @param numSamples  MC采样次数(默认20,越多越准确但越慢)
 * @return            估计胜率 [0.0, 1.0]
 */
float mcEvaluateAction(struct Ddz * pDdz, vector<int>& action,
                       int numSamples) {
    int wins = 0;

    // ──── 预处理: 将action转为带哨兵的数组 ────
    int actionArr[21];
    int actLen = 0;
    for (int a : action) {
        if (a < 0) break;  // 遇到哨兵(-1)停止
        actionArr[actLen++] = a;
    }
    actionArr[actLen] = -1;

    // 解析候选牌的牌型和等级(只需要算一次)
    int actionTypeCount = AnalyzeTypeCount(actionArr);
    int actionMainPoint = AnalyzeMainPoint(actionArr);

    // ──── 主循环: N次蒙特卡洛采样 ────
    for (int sample = 0; sample < numSamples; sample++) {
        // 1. 确定化采样: 随机分配未见牌
        vector<int> oppA_hand, oppB_hand;
        determinize(oppA_hand, oppB_hand);

        // 2. 设置初始场上状态(我出牌后)
        int lastTypeCount = actionTypeCount;
        int lastMainPoint = actionMainPoint;
        int lastPassCount = 0;

        // 3. 计算我的剩余手牌
        vector<int> myHand;
        for (int i = 0; pDdz->iOnHand[i] >= 0; i++) {
            bool played = false;
            for (int j = 0; j < actLen; j++) {
                if (pDdz->iOnHand[i] == actionArr[j]) {
                    played = true;
                    break;
                }
            }
            if (!played) myHand.push_back(pDdz->iOnHand[i]);
        }
        sort(myHand.begin(), myHand.end());

        // 如果出完牌直接获胜
        if (myHand.empty()) {
            wins++;
            continue;
        }

        // 4. 建立对手映射表
        int nextPlayer = (myPosition + 1) % 3;  // 下一个人出牌
        vector<int>* oppHands[2];
        int oppIds[2];
        if (myPosition == 0) {        // 我是地主
            oppHands[0] = &oppA_hand; oppIds[0] = 1;  // 农民A
            oppHands[1] = &oppB_hand; oppIds[1] = 2;  // 农民B
        } else if (myPosition == 1) { // 我是农民A
            oppHands[0] = &oppA_hand; oppIds[0] = 0;  // 地主
            oppHands[1] = &oppB_hand; oppIds[1] = 2;  // 农民B
        } else {                      // 我是农民B
            oppHands[0] = &oppA_hand; oppIds[0] = 0;  // 地主
            oppHands[1] = &oppB_hand; oppIds[1] = 1;  // 农民A
        }

        // 5. 模拟对战直到有人获胜或超时(80回合)
        int winner = -1;
        for (int turn = 0; turn < 80; turn++) {
            // 检查是否有人出完牌
            if (myHand.empty())     { winner = myPosition; break; }
            if (oppA_hand.empty())  { winner = oppIds[0];  break; }
            if (oppB_hand.empty())  { winner = oppIds[1];  break; }

            if (nextPlayer == myPosition) {
                // ──── 我的回合(用简化贪心策略) ────
                int counts[15] = {0};
                for (int c : myHand) counts[card2level(c)]++;

                bool played = false;
                int typeBase = lastTypeCount / 100;

                // 自由出牌 → 出最小的单牌
                if (lastPassCount >= 2 || lastTypeCount == 0) {
                    lastTypeCount = 301;
                    lastMainPoint = card2level(myHand[0]);
                    myHand.erase(myHand.begin());
                    lastPassCount = 0;
                    played = true;
                }
                // 尝试同类型压制: 单牌
                else if (typeBase == 3) {
                    for (int lv = lastMainPoint + 1; lv <= 14; lv++) {
                        if (counts[lv] >= 1) {
                            for (auto it = myHand.begin(); it != myHand.end(); ++it) {
                                if (card2level(*it) == lv) { myHand.erase(it); break; }
                            }
                            lastTypeCount = 301; lastMainPoint = lv;
                            lastPassCount = 0; played = true; break;
                        }
                    }
                }
                // 尝试同类型压制: 对子
                else if (typeBase == 4) {
                    for (int lv = lastMainPoint + 1; lv < 13; lv++) {
                        if (counts[lv] >= 2) {
                            int rm = 0;
                            for (auto it = myHand.begin(); it != myHand.end() && rm < 2; ) {
                                if (card2level(*it) == lv) { it = myHand.erase(it); rm++; }
                                else ++it;
                            }
                            lastTypeCount = 402; lastMainPoint = lv;
                            lastPassCount = 0; played = true; break;
                        }
                    }
                }

                // 尝试炸弹
                if (!played) {
                    for (int lv = 0; lv < 13; lv++) {
                        if (counts[lv] >= 4) {
                            int rm = 0;
                            for (auto it = myHand.begin(); it != myHand.end() && rm < 4; ) {
                                if (card2level(*it) == lv) { it = myHand.erase(it); rm++; }
                                else ++it;
                            }
                            lastTypeCount = 204; lastMainPoint = lv;
                            lastPassCount = 0; played = true; break;
                        }
                    }
                }
                // 尝试火箭
                if (!played && counts[13] >= 1 && counts[14] >= 1) {
                    for (auto it = myHand.begin(); it != myHand.end(); ) {
                        if (card2level(*it) == 13 || card2level(*it) == 14)
                            it = myHand.erase(it);
                        else ++it;
                    }
                    lastTypeCount = 102; lastMainPoint = 14;
                    lastPassCount = 0; played = true;
                }

                // PASS
                if (!played) {
                    lastPassCount++;
                    if (lastPassCount >= 2) {
                        lastTypeCount = 0; lastMainPoint = -1; lastPassCount = 0;
                    }
                }
            } else {
                // ──── 对手回合 ────
                int oppLocalIdx = (nextPlayer == oppIds[0]) ? 0 : 1;
                int result = simulateOpponentTurn(oppLocalIdx, *oppHands[oppLocalIdx],
                    lastTypeCount, lastMainPoint, lastPassCount);
                if (result >= 0) { winner = oppIds[oppLocalIdx]; break; }
            }

            nextPlayer = (nextPlayer + 1) % 3;
        }

        // 6. 判断我方是否获胜
        bool weWin = false;
        if (myPosition == 0) {            // 我是地主: 只有地主赢才算赢
            weWin = (winner == 0);
        } else {                          // 我是农民: 任一农民赢就算赢
            weWin = (winner == 1 || winner == 2);
        }
        // 超时(winner==-1)保守视为输

        if (weWin) wins++;
    }

    return (float)wins / (float)numSamples;  // 返回胜率
}

#endif // OPPONENT_MODEL_H
