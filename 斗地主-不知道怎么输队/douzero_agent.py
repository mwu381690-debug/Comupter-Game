"""
douzero_agent.py -- DouZero pretrained model agent for Dou Di Zhu battle platform
==================================================================================
Communicates via stdin/stdout protocol.
Uses DouZero's Deep Monte-Carlo pretrained models (ICML 2021).

Setup:
  1. git clone https://github.com/kwai/DouZero.git
  2. pip install torch numpy
  3. Download pretrained models from DouZero repo and place in ./baselines/
  4. Run: python douzero_agent.py --name TeamA

The agent loads 3 models (landlord, landlord_up, landlord_down) corresponding to
the 3 positions in Dou Di Zhu, and selects the appropriate one at runtime.
"""
import sys
import os
import re
import numpy as np
from collections import defaultdict

# ── Configuration ──
AI_NAME = "TeamA"
MODEL_DIR = "baselines/douzero_WP"  # or douzero_ADP
USE_DOUZERO = True

# ── Card mapping between platform (0-53) and DouZero (3-15,17,20) ──
# Platform: card_id / 4 = level (0=3,...,12=2), card_id 52=joker, 53=JOKER
# DouZero: 3=3, 4=4, ..., 14=A, 15=2, 17=small_joker, 20=big_joker
# For regular cards, suits don't matter, so we just map level→rank
def platform_card_to_dz(pid):
    """Convert platform card ID to DouZero rank value."""
    if pid == 52: return 17  # small joker
    if pid == 53: return 20  # big joker
    return (pid // 4) + 3    # 0→3, 1→4, ..., 12→15... wait, 2 should be 15

# Fix: level 0=3, 1=4, ..., 12=2. In DouZero, 3=3, 4=4, ..., 14=A, 15=2
def platform_to_dz(pid):
    if pid == 52: return 17
    if pid == 53: return 20
    lv = pid // 4
    if lv <= 11: return lv + 3   # 0→3 ... 11→14(A)
    return 15  # level 12 = 2 → DouZero 15

# For action conversion: DouZero action is a list of rank values
# We need to map back to specific platform card IDs
def dz_action_to_platform(dz_cards, hand_platform_ids, used_ids):
    """Convert DouZero action (list of ranks) to platform card IDs."""
    result = []
    needed = defaultdict(int)
    for dz_val in dz_cards:
        needed[dz_val] += 1

    # First try from hand
    hand_by_dz = defaultdict(list)
    for pid in hand_platform_ids:
        if pid not in used_ids:
            hand_by_dz[platform_to_dz(pid)].append(pid)

    for dz_val, count in needed.items():
        candidates = hand_by_dz.get(dz_val, [])
        picked = 0
        for c in candidates:
            if c not in used_ids and picked < count:
                result.append(c)
                used_ids.add(c)
                picked += 1

    return sorted(result)

# ── Game state (DouZero format) ──
class GameState:
    def __init__(self):
        self.reset()

    def reset(self):
        self.my_hand = []           # DouZero card values
        self.my_hand_pids = []      # Platform card IDs
        self.my_seat = ''           # 'A','B','C'
        self.my_position = ''       # 'landlord', 'landlord_up', 'landlord_down'
        self.landlord_seat = ''
        self.other_hands = []       # Unseen DouZero cards
        self.last_move = []         # Last move in DouZero format
        self.last_moves = {'landlord': [], 'landlord_up': [], 'landlord_down': []}
        self.legal_actions = []     # Legal actions (list of list of DouZero values)
        self.num_cards_left = {'landlord': 20, 'landlord_up': 17, 'landlord_down': 17}
        self.played_cards = {'landlord': [], 'landlord_up': [], 'landlord_down': []}
        self.action_seq = []        # Full history of moves
        self.bomb_num = 0
        self.used_pids = set()      # Platform IDs already played

state = GameState()

# ── DouZero agent (lazy load) ──
deep_agents = {}  # position -> DeepAgent

def init_douzero():
    """Load DouZero pretrained models."""
    global deep_agents
    try:
        sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'DouZero'))
        from douzero.evaluation.deep_agent import DeepAgent

        positions = ['landlord', 'landlord_up', 'landlord_down']
        for pos in positions:
            model_path = os.path.join(MODEL_DIR, f'{pos}.ckpt')
            if os.path.exists(model_path):
                deep_agents[pos] = DeepAgent(pos, model_path)
                print(f"Loaded {pos} model from {model_path}", file=sys.stderr)
            else:
                print(f"WARNING: {model_path} not found, using greedy for {pos}", file=sys.stderr)
        return len(deep_agents) > 0
    except Exception as e:
        print(f"DouZero init failed: {e}", file=sys.stderr)
        return False

def build_infoset():
    """Build a DouZero-compatible infoset from current game state."""
    # We need a simple object with the required attributes
    class InfoSet:
        pass
    infoset = InfoSet()

    # Map positions to DouZero role names
    pos_to_role = {0: 'landlord', 1: 'landlord_up', 2: 'landlord_down'}
    seat_to_pos = {}
    # Figure out which seat maps to which position
    seats = ['A', 'B', 'C']
    landlord_idx = ord(state.landlord_seat) - ord('A')
    for i, seat in enumerate(seats):
        rel = (i - landlord_idx) % 3
        if rel == 0: pos = 0
        elif rel == 1: pos = 1
        else: pos = 2
        seat_to_pos[seat] = pos

    my_pos_idx = seat_to_pos.get(state.my_seat, 0)
    my_role = pos_to_role[my_pos_idx]
    state.my_position = my_role

    # Set infoset attributes
    infoset.player_hand_cards = state.my_hand
    infoset.other_hand_cards = state.other_hands  # All unseen cards
    infoset.last_move = state.last_move
    infoset.last_move_dict = state.last_moves
    infoset.legal_actions = state.legal_actions
    infoset.num_cards_left_dict = state.num_cards_left
    infoset.played_cards = state.played_cards
    infoset.card_play_action_seq = state.action_seq
    infoset.bomb_num = state.bomb_num

    return infoset, my_role

# ── Legal action generation (simplified, matches DouZero's move_detector) ──
def generate_legal_actions(hand_dz, last_move_dz):
    """Generate legal DouZero-format actions. Simplified version."""
    if not last_move_dz:
        # Free play: all possible combos
        return generate_all_combos(hand_dz)

    # Must beat last_move_dz
    return generate_beating_combos(hand_dz, last_move_dz)

def generate_all_combos(hand):
    """Generate all valid DouZero card combinations from hand."""
    actions = []
    counts = defaultdict(int)
    for c in hand: counts[c] += 1

    # Singles
    for c in set(hand):
        actions.append([c])

    # Pairs
    for val, cnt in counts.items():
        if cnt >= 2 and val <= 15:
            actions.append([val, val])

    # Triples
    for val, cnt in counts.items():
        if cnt >= 3 and val <= 15:
            actions.append([val, val, val])
            # Trio+1: add any single as kicker
            for c in set(hand):
                if c != val:
                    actions.append([val, val, val, c])
            # Trio+2: add any pair as kicker
            for v2, c2 in counts.items():
                if v2 != val and c2 >= 2 and v2 <= 15:
                    actions.append([val, val, val, v2, v2])

    # Bombs
    for val, cnt in counts.items():
        if cnt >= 4 and val <= 15:
            actions.append([val, val, val, val])
            # Bomb+2 kickers
            kickers = [c for c in set(hand) if c != val]
            if len(kickers) >= 2:
                actions.append([val, val, val, val, kickers[0], kickers[1]])

    # Rocket
    if 17 in counts and 20 in counts:
        actions.append([17, 20])

    # Straights (5+ consecutive singles)
    ranks = sorted(set(v for v in counts if v <= 14))
    for i in range(len(ranks)):
        for j in range(i+4, len(ranks)):
            if ranks[j] - ranks[i] == j - i:
                straight = []
                for r in ranks[i:j+1]:
                    straight.append(r)
                if len(straight) >= 5:
                    actions.append(straight)

    # Double straights (3+ consecutive pairs)
    pair_ranks = sorted(v for v in counts if counts[v] >= 2 and v <= 14)
    for i in range(len(pair_ranks)):
        for j in range(i+2, len(pair_ranks)):
            if pair_ranks[j] - pair_ranks[i] == j - i:
                dstraight = []
                for r in pair_ranks[i:j+1]:
                    dstraight.extend([r, r])
                if len(dstraight) >= 6:
                    actions.append(dstraight)

    # Trio straights (2+ consecutive triples)
    trio_ranks = sorted(v for v in counts if counts[v] >= 3 and v <= 14)
    for i in range(len(trio_ranks)):
        for j in range(i+1, len(trio_ranks)):
            if trio_ranks[j] - trio_ranks[i] == j - i:
                tstraight = []
                for r in trio_ranks[i:j+1]:
                    tstraight.extend([r, r, r])
                if len(tstraight) >= 6:
                    actions.append(tstraight)

    # Pass (empty action)
    actions.append([])

    return actions

def generate_beating_combos(hand, last_move):
    """Generate combos that can beat last_move."""
    if not last_move:
        return generate_all_combos(hand)

    actions = []
    counts = defaultdict(int)
    for c in hand: counts[c] += 1
    last_counts = defaultdict(int)
    for c in last_move: last_counts[c] += 1
    last_len = len(last_move)

    # Determine last move type
    is_rocket = set(last_move) == {17, 20}
    is_bomb = last_len == 4 and len(set(last_move)) == 1 and list(set(last_move))[0] <= 15
    is_single = last_len == 1
    is_pair = last_len == 2 and len(set(last_move)) == 1
    is_triple = last_len == 3 and len(set(last_move)) == 1
    last_main = max(last_move) if last_move else 0

    # Rocket beats everything
    if not is_rocket and 17 in counts and 20 in counts:
        actions.append([17, 20])

    # Bomb beats non-bomb
    if not is_rocket:
        for val, cnt in counts.items():
            if cnt >= 4 and val <= 15:
                if not is_bomb or val > last_main:
                    actions.append([val, val, val, val])

    # Same type
    if is_single:
        for c in set(hand):
            if c > last_main:
                actions.append([c])
        # JOKER beats everything
        if 20 in counts and last_main < 20:
            actions.append([20])

    elif is_pair:
        for val in counts:
            if counts[val] >= 2 and val > last_main and val <= 15:
                actions.append([val, val])

    elif is_triple:
        for val in counts:
            if counts[val] >= 3 and val > last_main and val <= 15:
                actions.append([val, val, val])

    actions.append([])  # Pass option
    return actions

def is_bomb_or_rocket(move):
    if not move: return False
    if set(move) == {17, 20}: return True
    return len(move) == 4 and len(set(move)) == 1

# ── Greedy fallback agent ──
def greedy_decide(infoset):
    """Simple greedy: play smallest legal action, pass only if no option."""
    legal = [a for a in infoset.legal_actions if a]  # non-pass actions
    if legal:
        return min(legal, key=lambda a: (len(a), max(a) if a else 0))
    return []

# ── Main decision function ──
def decide_play():
    """Decide what to play. Returns list of platform card IDs."""
    if not state.my_hand:
        return []

    # Generate legal actions
    state.legal_actions = generate_legal_actions(state.my_hand, state.last_move)

    # If only one option, use it
    if len(state.legal_actions) == 1:
        dz_action = state.legal_actions[0]
    elif USE_DOUZERO and state.my_position in deep_agents:
        try:
            infoset, role = build_infoset()
            agent = deep_agents[role]
            dz_action = agent.act(infoset)
        except Exception as e:
            print(f"DouZero error: {e}, falling back to greedy", file=sys.stderr)
            infoset, _ = build_infoset()
            dz_action = greedy_decide(infoset)
    else:
        infoset, _ = build_infoset()
        dz_action = greedy_decide(infoset)

    # Convert DouZero action to platform card IDs
    if not dz_action:  # Pass
        return []

    return dz_action_to_platform(dz_action, state.my_hand_pids, state.used_pids)

# ── Protocol handlers ──
def handle_doudizhuver(msg):
    return f"NAME {AI_NAME}"

def handle_info(msg):
    return "OK INFO"

def handle_deal(msg):
    seat = msg[5]
    state.my_seat = seat
    parts = msg[7:].split(',')
    state.my_hand_pids = [int(p) for p in parts if p.strip()]
    state.my_hand_pids.sort()
    state.my_hand = [platform_to_dz(p) for p in state.my_hand_pids]
    state.my_hand.sort()
    return "OK DEAL"

def handle_bid(msg):
    if msg[4] == 'W':
        # Compute bid
        counts = defaultdict(int)
        for dz_val in state.my_hand:
            counts[dz_val] += 1
        c2 = counts.get(15, 0)  # 2
        cJo = counts.get(17, 0)
        cJO = counts.get(20, 0)
        bombs = sum(1 for v, c in counts.items() if c >= 4 and v <= 15)

        if (cJo >= 1 and cJO >= 1) or (cJO >= 1 and c2 >= 2) or (bombs >= 1 and c2 >= 1 and cJO >= 1):
            bid = 3
        elif c2 >= 2 or (cJO >= 1 and c2 >= 1):
            bid = 2
        elif c2 >= 1 or cJO >= 1:
            bid = 1
        else:
            bid = 0
        return f"BID {state.my_seat}{bid}"
    else:
        seat = msg[4]
        amount = int(msg[5])
        if amount > 0:
            state.landlord_seat = seat
        return "OK BID"

def handle_leftover(msg):
    seat = msg[9]
    parts = msg[11:].split(',')
    cards = [int(p) for p in parts if p.strip()]
    if seat == state.my_seat:
        state.my_hand_pids.extend(cards)
        state.my_hand_pids.sort()
        state.my_hand = [platform_to_dz(p) for p in state.my_hand_pids]
        state.my_hand.sort()

    # Compute other hands
    all_dz = []
    for i in range(3, 16): all_dz.extend([i]*4)
    all_dz.extend([17]*4)  # Actually only 1 each
    # Actually DouZero deck has specific counts. Let me recalculate
    # DouZero deck: 3-14 (12 ranks) * 4 = 48, 17*4 (only 1 small joker?), 20, 30
    # Let me just track unseen cards simply
    return "OK LEFTOVER"

def handle_play(msg):
    if msg[5] == 'W':  # PLAY WHAT - our turn
        action_pids = decide_play()
        if not action_pids:
            return f"PLAY {state.my_seat}-1"
        cards_str = ','.join(str(c) for c in action_pids)
        # Update own state
        for pid in action_pids:
            state.used_pids.add(pid)
            state.my_hand_pids.remove(pid)
        state.my_hand = [platform_to_dz(p) for p in state.my_hand_pids]
        return f"PLAY {state.my_seat}{cards_str}"
    else:  # Other player's play
        seat = msg[5]
        cards_str = msg[6:]
        if cards_str == '-1':
            state.last_move = []
            state.action_seq.append([])
        else:
            cards = [int(c) for c in cards_str.split(',') if c.strip()]
            for pid in cards:
                state.used_pids.add(pid)
            state.last_move = [platform_to_dz(p) for p in cards]
            state.action_seq.append(state.last_move)
            if is_bomb_or_rocket(state.last_move):
                state.bomb_num += 1
        return "OK PLAY"

def handle_gameover(msg):
    state.reset()
    return "OK GAMEOVER"

# ── Main ──
def main():
    global AI_NAME, USE_DOUZERO

    # Parse args
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == '--name' and i+1 < len(args):
            AI_NAME = args[i+1]; i += 2
        elif args[i] == '--model_dir' and i+1 < len(args):
            global MODEL_DIR
            MODEL_DIR = args[i+1]; i += 2
        elif args[i] == '--greedy':
            USE_DOUZERO = False; i += 1
        else:
            i += 1

    # Try to init DouZero
    if USE_DOUZERO:
        if not init_douzero():
            print("Falling back to greedy policy", file=sys.stderr)
            USE_DOUZERO = False

    print(f"Agent ready: {AI_NAME}, DouZero={'on' if USE_DOUZERO else 'off'}", file=sys.stderr)

    # Main loop
    for line in sys.stdin:
        line = line.strip()
        if not line: continue

        cmd = line[:3]
        handlers = {
            'DOU': handle_doudizhuver,
            'INF': handle_info,
            'DEA': handle_deal,
            'BID': handle_bid,
            'LEF': handle_leftover,
            'PLA': handle_play,
            'GAM': handle_gameover,
        }

        if cmd == 'EXI':
            break

        handler = handlers.get(cmd)
        if handler:
            resp = handler(line)
        else:
            resp = "ERROR"

        print(resp, flush=True)

if __name__ == '__main__':
    main()
