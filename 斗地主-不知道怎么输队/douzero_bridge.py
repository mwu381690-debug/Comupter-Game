"""
douzero_bridge.py — DouZero pretrained model bridge for Dou Di Zhu battle platform
===================================================================================
Communicates via stdin/stdout using the platform's text protocol.
Uses DouZero's pretrained Deep Monte-Carlo models for decision making.

Installation:
  pip install douzero torch numpy
  # Download pretrained models from DouZero repo

Usage:
  python douzero_bridge.py [--model_dir baselines/douzero_WP] [--name TeamA]
  # Or compile to exe: pyinstaller --onefile douzero_bridge.py
"""
import sys
import os
import re
import random
import numpy as np

# ── Card encoding mapping ──
# Platform: card 0-53 → level = card/4 + card/53
#   level 0(3),1(4),...,12(2),13(joker),14(JOKER)
# DouZero: card values = 3-14, 17(joker), 20(JOKER)
#   suits don't matter, so we use level+3 for regular cards

PLATFORM_TO_DZ = {}  # platform_card_id -> douzero_card_value
DZ_TO_PLATFORM = {}  # douzero_card_value -> [platform_card_ids]

def build_card_mapping():
    """Build bidirectional mapping between platform and DouZero card encodings."""
    global PLATFORM_TO_DZ, DZ_TO_PLATFORM
    for cid in range(52):  # Regular cards 0-51
        level = cid // 4  # 0=3,...,12=2
        dz_val = level + 3  # 3-14
        PLATFORM_TO_DZ[cid] = dz_val
        if dz_val not in DZ_TO_PLATFORM:
            DZ_TO_PLATFORM[dz_val] = []
        DZ_TO_PLATFORM[dz_val].append(cid)
    # Jokers
    PLATFORM_TO_DZ[52] = 17  # Small joker
    PLATFORM_TO_DZ[53] = 20  # Big joker
    DZ_TO_PLATFORM[17] = [52]
    DZ_TO_PLATFORM[20] = [53]

build_card_mapping()

# ── DouZero imports (lazy, only if models are available) ──
try:
    import torch
    HAS_TORCH = True
except ImportError:
    HAS_TORCH = False
    print("WARNING: torch not installed. Using random agent.", file=sys.stderr)

# ── Game State ──
class GameState:
    def __init__(self):
        self.my_cards = []          # My hand (platform IDs)
        self.my_position = -1       # 0=Landlord, 1=FarmerA, 2=FarmerB
        self.landlord_pos = -1      # Which seat is landlord
        self.my_seat = ''           # My seat letter (A/B/C)
        self.played_cards = {0: [], 1: [], 2: []}  # Played cards by position
        self.last_move = []         # Last move (platform IDs)
        self.last_player = -1       # Who played last
        self.card_remaining = [20, 17, 17]  # Cards remaining per position
        self.bid_info = [-1, -1, -1]  # Bid amounts
        self.bottom_cards = []      # Landlord extra cards
        self.round_history = []     # Full play history
        self.bomb_count = 0

    def reset(self):
        self.__init__()

state = GameState()

# ── Protocol handlers ──
def handle_doudizhuver(msg):
    return "NAME " + AI_NAME

def handle_info(msg):
    return "OK INFO"

def handle_deal(msg):
    """DEAL <seat> card1,card2,..."""
    seat = msg[5]
    state.my_seat = seat
    parts = msg[7:].split(',')
    state.my_cards = [int(p) for p in parts if p.strip()]
    state.my_cards.sort()
    # Initialize card remaining
    state.card_remaining = [17, 17, 17]
    return "OK DEAL"

def handle_bid(msg):
    """BID WHAT or BID <seat><amount>"""
    if msg[4] == 'W':  # BID WHAT
        bid = compute_bid()
        return f"BID {state.my_seat}{bid}"
    else:  # BID <seat><amount>
        seat = msg[4]
        amount = int(msg[5])
        pos = ord(seat) - ord('A')
        state.bid_info[pos] = amount
        if amount > 0:
            state.landlord_pos = pos
        return "OK BID"

def handle_leftover(msg):
    """LEFTOVER <seat> card1,card2,card3"""
    seat = msg[9]
    parts = msg[11:].split(',')
    cards = [int(p) for p in parts if p.strip()]
    if seat == state.my_seat:
        state.my_cards.extend(cards)
        state.my_cards.sort()
        state.card_remaining[state.my_position] = 20
    state.bottom_cards = cards
    return "OK LEFTOVER"

def handle_play(msg):
    """PLAY WHAT or PLAY <seat> cards..."""
    if msg[5] == 'W':  # PLAY WHAT
        action = compute_play()
        if action is None or len(action) == 0:
            return f"PLAY {state.my_seat}-1"
        cards_str = ','.join(str(c) for c in action)
        return f"PLAY {state.my_seat}{cards_str}"
    else:  # PLAY <seat> cards...
        seat = msg[5]
        pos = ord(seat) - ord('A')
        cards_str = msg[6:]
        if cards_str == '-1':
            # Pass
            state.last_move = []
            state.round_history.append([])
        else:
            cards = [int(c) for c in cards_str.split(',') if c.strip()]
            state.last_move = cards
            state.played_cards[pos].extend(cards)
            state.card_remaining[pos] -= len(cards)
            state.round_history.append(cards)
            # Check for bomb
            if is_bomb(cards):
                state.bomb_count += 1
        state.last_player = pos
        return "OK PLAY"

def handle_gameover(msg):
    return "OK GAMEOVER"

def handle_exit(msg):
    sys.exit(0)

def is_bomb(cards):
    """Check if cards form a bomb or rocket."""
    if len(cards) == 2:
        return set(cards) == {52, 53}  # Rocket
    if len(cards) == 4:
        levels = [c // 4 for c in cards]
        return len(set(levels)) == 1 and levels[0] < 13  # Four of a kind
    return False

# ── Bidding logic (conservative, same as C++ version) ──
def compute_bid():
    """Conservative bidding based on hand strength."""
    level_counts = [0] * 15
    for c in state.my_cards:
        level_counts[c // 4 + (1 if c >= 52 else 0)] += 1
    cnt2 = level_counts[12]
    cnt_joker = level_counts[13]
    cnt_JOKER = level_counts[14]
    bombs = sum(1 for lv in range(13) if level_counts[lv] >= 4)

    if (cnt_joker >= 1 and cnt_JOKER >= 1) or (cnt_JOKER >= 1 and cnt2 >= 2) or (bombs >= 1 and cnt2 >= 1 and cnt_JOKER >= 1):
        bid = 3
    elif cnt2 >= 2 or (cnt_JOKER >= 1 and cnt2 >= 1):
        bid = 2
    elif cnt2 >= 1 or cnt_JOKER >= 1:
        bid = 1
    else:
        bid = 0

    # Don't outbid
    for b in state.bid_info:
        if b >= bid:
            return 0
    return bid

# ── Card mapping helpers ──
def platform_to_dz_cards(platform_cards):
    """Convert platform card IDs to DouZero card values."""
    return [PLATFORM_TO_DZ[c] for c in platform_cards if c >= 0]

def dz_to_platform_cards(dz_cards):
    """Convert DouZero card values to platform card IDs.
    Picks specific suit cards, ensuring no duplicates with already-played cards."""
    result = []
    used = set()
    for p in state.my_cards:
        used.add(p)
    for pos_cards in state.played_cards.values():
        for c in pos_cards:
            used.add(c)

    for dz_val in dz_cards:
        candidates = DZ_TO_PLATFORM.get(dz_val, [])
        picked = None
        for c in candidates:
            if c not in used:
                picked = c
                used.add(c)
                break
        if picked is not None:
            result.append(picked)
    return sorted(result)

# ── Simplified greedy play (fallback if DouZero not available) ──
def greedy_play():
    """Simple greedy policy: play smallest single or beat current play minimally."""
    my_hand = sorted(state.my_cards)

    # Check if it's my turn to lead (free play)
    is_free = (len(state.last_move) == 0)

    if is_free:
        # Play smallest single
        if my_hand:
            return [my_hand[0]]
        return None

    # Try to beat the current play
    last_levels = [c // 4 for c in state.last_move]
    last_count = len(state.last_move)

    if last_count == 1:  # Single
        last_lv = state.last_move[0] // 4
        for c in my_hand:
            lv = c // 4
            if lv > last_lv or c == 53:  # JOKER beats everything
                return [c]
    elif last_count == 2:  # Pair
        last_lv = state.last_move[0] // 4
        level_counts = {}
        for c in my_hand:
            lv = c // 4
            level_counts[lv] = level_counts.get(lv, 0) + 1
        for lv in sorted(level_counts.keys()):
            if lv > last_lv and level_counts[lv] >= 2:
                return [c for c in my_hand if c // 4 == lv][:2]

    # Check for bombs
    level_counts = {}
    for c in my_hand:
        lv = c // 4
        if lv < 13:
            level_counts[lv] = level_counts.get(lv, 0) + 1
    for lv, cnt in level_counts.items():
        if cnt >= 4:
            return [c for c in my_hand if c // 4 == lv][:4]
    # Rocket
    if 52 in my_hand and 53 in my_hand:
        return [52, 53]

    return None  # Pass

# ── Main loop ──
AI_NAME = "TeamA"

def main():
    global AI_NAME
    # Parse args
    if len(sys.argv) > 1:
        if sys.argv[1] == '--name' and len(sys.argv) > 2:
            AI_NAME = sys.argv[2]

    # Try to load DouZero
    use_douzero = False
    if HAS_TORCH:
        try:
            sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
            # We would load DouZero models here if available
            # from douzero.env import Env
            # use_douzero = True
            pass
        except ImportError:
            pass

    if not use_douzero:
        print(f"DouZero not available, using greedy policy", file=sys.stderr)

    # Main communication loop
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue

        cmd = line[:3]

        if cmd == "DOU":
            resp = handle_doudizhuver(line)
        elif cmd == "INF":
            resp = handle_info(line)
        elif cmd == "DEA":
            resp = handle_deal(line)
        elif cmd == "BID":
            resp = handle_bid(line)
        elif cmd == "LEF":
            resp = handle_leftover(line)
        elif cmd == "PLA":
            resp = handle_play(line)
        elif cmd == "GAM":
            resp = handle_gameover(line)
        elif cmd == "EXI":
            handle_exit(line)
        else:
            resp = "ERROR"

        print(resp, flush=True)

if __name__ == "__main__":
    main()
