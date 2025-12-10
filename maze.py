import struct
import pygame
import sys
import os
from collections import deque
from enum import Enum, auto

# ==========================================
# 1. ENUMS & CONSTANTS
# ==========================================

class Direction(Enum):
    North = 0 
    East = 1
    South = 2
    West = 3
    Uninitialized = 4

class InternalBit(Enum):
    EAST_BIT = 1
    SOUTH_BIT = 2
    VISITED_BIT = 4     
    PATH_BIT = 8        
    
    PARENT_NORTH = 16
    PARENT_EAST  = 32
    PARENT_SOUTH = 64
    PARENT_WEST  = 128
    PARENT_MASK  = 240 
    
    ON_STACK_BIT = 256
    
    # MT - Branch Logic
    DEAD_N = 0x1000
    DEAD_E = 0x2000
    DEAD_S = 0x4000
    DEAD_W = 0x8000
    
    OCCUPIED_N = 0x10000
    OCCUPIED_E = 0x20000
    OCCUPIED_S = 0x40000
    OCCUPIED_W = 0x80000

    # Team Ownership
    VISITED_TB = 0x100000 
    VISITED_BT = 0x200000 
    
    # Dead State (Dark Gray) - ONLY for True Junctions
    DEAD_JUNCTION_BIT = 0x400000 
    
    # MT_M1 Specific
    PRUNED_BIT = 0x800000

# ==========================================
# 2. HELPER CLASSES
# ==========================================

class Position:
    def __init__(self, row=0, col=0):
        self.row = row
        self.col = col
    def __eq__(self, other): return self.row == other.row and self.col == other.col
    def __ne__(self, other): return not self.__eq__(other)
    def __hash__(self): return hash((self.row, self.col))
    def move(self, dir: Direction):
        if dir == Direction.North: return Position(self.row - 1, self.col)
        if dir == Direction.South: return Position(self.row + 1, self.col)
        if dir == Direction.East:  return Position(self.row, self.col + 1)
        if dir == Direction.West:  return Position(self.row, self.col - 1)
        return self
    def __repr__(self): return f"({self.row}, {self.col})"

def reverseDir(d: Direction):
    if d == Direction.North: return Direction.South
    if d == Direction.South: return Direction.North
    if d == Direction.East: return Direction.West
    if d == Direction.West: return Direction.East
    return Direction.Uninitialized

# ==========================================
# 3. UI ELEMENTS
# ==========================================

class SimpleSlider:
    def __init__(self, x, y, w, h, min_val, max_val, start_val):
        self.rect = pygame.Rect(x, y, w, h)
        self.min_val = min_val
        self.max_val = max_val
        self.val = start_val
        self.dragging = False
        self.knob_w = 20
        self.knob_h = h + 10
        self.knob_rect = pygame.Rect(x, y - 5, self.knob_w, self.knob_h)
        self.update_knob_pos()

    def update_knob_pos(self):
        ratio = (self.val - self.min_val) / (self.max_val - self.min_val)
        self.knob_rect.centerx = self.rect.x + (self.rect.width * ratio)

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if self.knob_rect.collidepoint(event.pos) or self.rect.collidepoint(event.pos):
                self.dragging = True
                self.update_value(event.pos[0])
        elif event.type == pygame.MOUSEBUTTONUP:
            self.dragging = False
        elif event.type == pygame.MOUSEMOTION and self.dragging:
            self.update_value(event.pos[0])

    def update_value(self, mouse_x):
        x = max(self.rect.x, min(mouse_x, self.rect.right))
        ratio = (x - self.rect.x) / self.rect.width
        self.val = self.min_val + (ratio * (self.max_val - self.min_val))
        self.update_knob_pos()

    def draw(self, screen):
        pygame.draw.rect(screen, (180, 180, 180), self.rect, border_radius=5)
        fill_rect = pygame.Rect(self.rect.x, self.rect.y, self.knob_rect.centerx - self.rect.x, self.rect.height)
        pygame.draw.rect(screen, (100, 100, 200), fill_rect, border_radius=5)
        pygame.draw.rect(screen, (50, 50, 150), self.knob_rect, border_radius=5)
        pygame.draw.rect(screen, (255, 255, 255), self.knob_rect, 1, border_radius=5)

    def get_value(self): return self.val

class SimpleButton:
    def __init__(self, x, y, w, h, text, callback, color=(200, 50, 50)):
        self.rect = pygame.Rect(x, y, w, h)
        self.text = text
        self.callback = callback
        self.font = pygame.font.SysFont('Arial', 14, bold=True)
        self.bg_color = color
        self.hover_color = (min(color[0]+30, 255), min(color[1]+30, 255), min(color[2]+30, 255))
        self.active_color = (0, 150, 0)
        self.is_hovered = False
        self.is_active = False

    def handle_event(self, event):
        if event.type == pygame.MOUSEMOTION:
            self.is_hovered = self.rect.collidepoint(event.pos)
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if self.rect.collidepoint(event.pos):
                self.callback()

    def draw(self, screen):
        color = self.active_color if self.is_active else (self.hover_color if self.is_hovered else self.bg_color)
        pygame.draw.rect(screen, color, self.rect, border_radius=5)
        pygame.draw.rect(screen, (0, 0, 0), self.rect, 2, border_radius=5)
        text_surf = self.font.render(self.text, True, (255, 255, 255))
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)

# ==========================================
# 4. MAZE & SHARED DATA
# ==========================================

class Maze:
    def __init__(self):
        self.width = 0
        self.height = 0
        self.poMazeData = []
        self.visitOrder = []
        self.thread_ownership = {} 

    def Load(self, filename):
        with open(filename, 'rb') as f:
            header_bytes = f.read(12)
            self.width, self.height, solvable = struct.unpack('iii', header_bytes)
            self.poMazeData = [0] * (self.width * self.height)
            self.visitOrder = [-1] * (self.width * self.height)
            
            int_size = 4
            for row in range(self.height):
                col = 0
                while col < self.width:
                    int_bytes = f.read(int_size)
                    if len(int_bytes) < int_size: break
                    bits, = struct.unpack('i', int_bytes)
                    for _ in range(16):
                        if col >= self.width: break
                        east = bits & 1
                        south = (bits >> 1) & 1
                        idx = row * self.width + col
                        val = 0
                        if east: val |= InternalBit.EAST_BIT.value
                        if south: val |= InternalBit.SOUTH_BIT.value
                        self.poMazeData[idx] = val
                        bits >>= 2
                        col += 1
    
    def Reset(self):
        mask = ~(InternalBit.VISITED_BIT.value | 
                 InternalBit.PATH_BIT.value | 
                 InternalBit.PARENT_MASK.value |
                 InternalBit.ON_STACK_BIT.value |
                 InternalBit.PRUNED_BIT.value |
                 0xFFFF000 ) 
        for i in range(len(self.poMazeData)):
            self.poMazeData[i] &= mask
            self.visitOrder[i] = -1
        self.thread_ownership = {}

    def setVisitOrder(self, pos: Position, order: int):
        idx = self._cellIndex(pos)
        self.visitOrder[idx] = order
    def getVisitOrder(self, pos: Position) -> int:
        idx = self._cellIndex(pos)
        return self.visitOrder[idx]
    def getStart(self) -> Position: return Position(0, self.width // 2)
    def getEnd(self) -> Position: return Position(self.height - 1, self.width // 2)
    def _cellIndex(self, pos: Position) -> int: return pos.row * self.width + pos.col
    def getCell(self, pos: Position) -> int:
        if 0 <= pos.row < self.height and 0 <= pos.col < self.width:
            return self.poMazeData[self._cellIndex(pos)]
        return 0
    def setCell(self, pos: Position, value: int):
        self.poMazeData[self._cellIndex(pos)] = value
    def _hasFlag(self, pos: Position, val) -> bool: return (self.getCell(pos) & val) != 0
    def _setFlag(self, pos: Position, val): self.setCell(pos, self.getCell(pos) | val)
    def _clearFlag(self, pos: Position, val): self.setCell(pos, self.getCell(pos) & ~val)

    def canMove(self, pos: Position, direction: Direction) -> bool:
        if direction == Direction.North:
            if pos.row == 0: return False
            return not self._hasFlag(pos.move(Direction.North), InternalBit.SOUTH_BIT.value)
        elif direction == Direction.South:
            if pos.row == self.height - 1: return False
            return not self._hasFlag(pos, InternalBit.SOUTH_BIT.value)
        elif direction == Direction.East:
            if pos.col == self.width - 1: return False
            return not self._hasFlag(pos, InternalBit.EAST_BIT.value)
        elif direction == Direction.West:
            if pos.col == 0: return False
            return not self._hasFlag(pos.move(Direction.West), InternalBit.EAST_BIT.value)
        return False

    def isJunction(self, pos: Position) -> bool:
        exits = 0
        if self.canMove(pos, Direction.North): exits += 1
        if self.canMove(pos, Direction.South): exits += 1
        if self.canMove(pos, Direction.East): exits += 1
        if self.canMove(pos, Direction.West): exits += 1
        return exits > 2

    # --- Standard BFS/DFS Helpers ---
    def setDirectionRouteBT(self, pos: Position, parent_dir: Direction):
        val = self.getCell(pos)
        val &= ~InternalBit.PARENT_MASK.value 
        val |= InternalBit.VISITED_BIT.value  
        if parent_dir == Direction.North: val |= InternalBit.PARENT_NORTH.value
        elif parent_dir == Direction.East: val |= InternalBit.PARENT_EAST.value
        elif parent_dir == Direction.South: val |= InternalBit.PARENT_SOUTH.value
        elif parent_dir == Direction.West: val |= InternalBit.PARENT_WEST.value
        self.setCell(pos, val)

    def getDirectionRouteBT(self, pos: Position) -> Direction:
        val = self.getCell(pos)
        if val & InternalBit.PARENT_NORTH.value: return Direction.North
        if val & InternalBit.PARENT_EAST.value: return Direction.East
        if val & InternalBit.PARENT_SOUTH.value: return Direction.South
        if val & InternalBit.PARENT_WEST.value: return Direction.West
        return Direction.Uninitialized

    def markPath(self, pos: Position):
        self._setFlag(pos, InternalBit.PATH_BIT.value)
    
    def markOnStack(self, pos: Position, on_stack: bool):
        if on_stack: self._setFlag(pos, InternalBit.ON_STACK_BIT.value)
        else: self._clearFlag(pos, InternalBit.ON_STACK_BIT.value)

    # --- MT SPECIFIC BIT OPERATIONS ---
    
    def checkBranchDead(self, pos: Position, dir: Direction) -> bool:
        bit = 0
        if dir == Direction.North: bit = InternalBit.DEAD_N.value
        elif dir == Direction.East: bit = InternalBit.DEAD_E.value
        elif dir == Direction.South: bit = InternalBit.DEAD_S.value
        elif dir == Direction.West: bit = InternalBit.DEAD_W.value
        return self._hasFlag(pos, bit)

    def setBranchDeadFromOrigin(self, pos: Position, dir: Direction):
        bit = 0
        if dir == Direction.North: bit = InternalBit.DEAD_N.value
        elif dir == Direction.East: bit = InternalBit.DEAD_E.value
        elif dir == Direction.South: bit = InternalBit.DEAD_S.value
        elif dir == Direction.West: bit = InternalBit.DEAD_W.value
        self._setFlag(pos, bit)

    def checkBranchOccupied(self, pos: Position, dir: Direction) -> bool:
        bit = 0
        if dir == Direction.North: bit = InternalBit.OCCUPIED_N.value
        elif dir == Direction.East: bit = InternalBit.OCCUPIED_E.value
        elif dir == Direction.South: bit = InternalBit.OCCUPIED_S.value
        elif dir == Direction.West: bit = InternalBit.OCCUPIED_W.value
        return self._hasFlag(pos, bit)

    def setBranchOccupied(self, pos: Position, dir: Direction):
        bit = 0
        if dir == Direction.North: bit = InternalBit.OCCUPIED_N.value
        elif dir == Direction.East: bit = InternalBit.OCCUPIED_E.value
        elif dir == Direction.South: bit = InternalBit.OCCUPIED_S.value
        elif dir == Direction.West: bit = InternalBit.OCCUPIED_W.value
        self._setFlag(pos, bit)

    def markVisitedTeam(self, pos: Position, is_tb: bool):
        if is_tb: self._setFlag(pos, InternalBit.VISITED_TB.value)
        else: self._setFlag(pos, InternalBit.VISITED_BT.value)

    def markDeadJunction(self, pos: Position):
        self._clearFlag(pos, InternalBit.VISITED_TB.value)
        self._clearFlag(pos, InternalBit.VISITED_BT.value)
        self._setFlag(pos, InternalBit.DEAD_JUNCTION_BIT.value)

    def unmarkVisitedTeam(self, pos: Position):
        self._clearFlag(pos, InternalBit.VISITED_TB.value)
        self._clearFlag(pos, InternalBit.VISITED_BT.value)
        
        # === FIX: ALSO CLEAR THE GENERIC VISITED BIT TO REMOVE GREY TRAIL ===
        self._clearFlag(pos, InternalBit.VISITED_BIT.value)

    def isVisitedByTeam(self, pos: Position, is_tb: bool):
        if is_tb: return self._hasFlag(pos, InternalBit.VISITED_TB.value)
        else: return self._hasFlag(pos, InternalBit.VISITED_BT.value)
        
    def setThreadOwner(self, pos: Position, thread_id: int):
        self.thread_ownership[self._cellIndex(pos)] = thread_id
    
    def getThreadOwner(self, pos: Position):
        return self.thread_ownership.get(self._cellIndex(pos), -1)
        
    # --- MT_M1 Helper ---
    def markPruned(self, pos: Position):
        self._setFlag(pos, InternalBit.PRUNED_BIT.value)
        
    def isPruned(self, pos: Position):
        return self._hasFlag(pos, InternalBit.PRUNED_BIT.value)

    def getAvailableMovesNoPruned(self, pos: Position):
        moves = []
        for d in [Direction.North, Direction.South, Direction.East, Direction.West]:
            if self.canMove(pos, d):
                n_pos = pos.move(d)
                if not self.isPruned(n_pos):
                    moves.append(d)
        return moves


# --- Ported Logic ---

class Branches:
    def __init__(self, maze: Maze, pos: Position, threadID: int):
        self.directions = [Direction.Uninitialized] * 4
        self.count = 0
        self.index = threadID & 3
        
        idx = 0
        for d in [Direction.North, Direction.East, Direction.South, Direction.West]:
            if maze.canMove(pos, d):
                self.directions[idx] = d
                self.count += 1
            else:
                self.directions[idx] = Direction.Uninitialized
            idx += 1

    def remove(self, d: Direction):
        for i in range(4):
            if self.directions[i] == d:
                self.directions[i] = Direction.Uninitialized
                self.count -= 1
                return

    def getNextThreads(self, at: Position, maze: Maze):
        if self.count == 0: return Direction.Uninitialized
        
        fallbackIndex = -1
        
        for _ in range(4):
            self.index = (self.index + 1) & 3
            d = self.directions[self.index]
            
            if d == Direction.Uninitialized: continue
            
            if maze.checkBranchDead(at, d):
                self.directions[self.index] = Direction.Uninitialized
                self.count -= 1
                if self.count == 0: return Direction.Uninitialized
                continue
            
            if fallbackIndex == -1: fallbackIndex = self.index
            
            if maze.checkBranchOccupied(at, d):
                continue
            
            maze.setBranchOccupied(at, d)
            return d
            
        if fallbackIndex != -1:
            self.index = fallbackIndex
            return self.directions[self.index]
            
        return Direction.Uninitialized

    def popCurrThreads(self, at: Position, maze: Maze):
        d = self.directions[self.index]
        if d != Direction.Uninitialized:
            maze.setBranchDeadFromOrigin(at, d)
            self.directions[self.index] = Direction.Uninitialized
            self.count -= 1
        return d
    
    def getNext(self):
        if self.count == 0: return Direction.Uninitialized
        for _ in range(4):
            self.index = (self.index + 1) & 3
            if self.directions[self.index] != Direction.Uninitialized:
                return self.directions[self.index]
        return Direction.Uninitialized

    def size(self): return self.count


class Junction:
    def __init__(self, at: Position, came_from: Direction, branches: Branches, isOverlap=False):
        self.at = at
        # This direction points BACK to the parent in the DFS tree
        self.came_from = came_from 
        self.branches = branches
        self.isOverlap = isOverlap

# ==========================================
# 5. SOLVERS
# ==========================================

class BFSSolver:
    def __init__(self, maze: Maze):
        self.maze = maze
    def solve_step_by_step(self):
        start = self.maze.getStart()
        end = self.maze.getEnd()
        q = deque([start])
        visit_counter = 0
        self.maze.setVisitOrder(start, visit_counter)
        visit_counter += 1
        self.maze.setDirectionRouteBT(start, Direction.Uninitialized)
        found = False
        while q:
            yield "SEARCHING"
            cur = q.popleft()
            if cur == end:
                found = True
                break
            for d in [Direction.South, Direction.West, Direction.East, Direction.North]:
                if self.maze.canMove(cur, d):
                    nextPos = cur.move(d)
                    if self.maze.getVisitOrder(nextPos) == -1:
                        parent = Direction.Uninitialized
                        if d == Direction.North: parent = Direction.South
                        elif d == Direction.South: parent = Direction.North
                        elif d == Direction.East: parent = Direction.West
                        elif d == Direction.West: parent = Direction.East
                        self.maze.setDirectionRouteBT(nextPos, parent)
                        self.maze.setVisitOrder(nextPos, visit_counter)
                        visit_counter += 1
                        q.append(nextPos)
        if found:
            curr = end
            while True:
                self.maze.markPath(curr)
                yield "BACKTRACKING"
                if curr == start: break
                parent_dir = self.maze.getDirectionRouteBT(curr)
                if parent_dir == Direction.Uninitialized: break 
                curr = curr.move(parent_dir)
            yield "FINISHED"
        else:
            yield "NO_SOLUTION"

class DFSSolver:
    def __init__(self, maze: Maze):
        self.maze = maze
    def solve_step_by_step(self):
        start = self.maze.getStart()
        end = self.maze.getEnd()
        stack = [(start, Direction.Uninitialized)]
        visit_counter = 0
        self.maze.setVisitOrder(start, visit_counter)
        visit_counter += 1
        self.maze.markOnStack(start, True)
        self.maze.setDirectionRouteBT(start, Direction.Uninitialized)
        found = False
        while stack:
            curr, _ = stack[-1]
            yield "SEARCHING"
            if curr == end:
                found = True
                break
            moved = False
            for d in [Direction.South, Direction.East, Direction.West, Direction.North]:
                if self.maze.canMove(curr, d):
                    nextPos = curr.move(d)
                    if self.maze.getVisitOrder(nextPos) == -1:
                        self.maze.setVisitOrder(nextPos, visit_counter)
                        visit_counter += 1
                        self.maze.markOnStack(nextPos, True)
                        parent = Direction.Uninitialized
                        if d == Direction.North: parent = Direction.South
                        elif d == Direction.South: parent = Direction.North
                        elif d == Direction.East: parent = Direction.West
                        elif d == Direction.West: parent = Direction.East
                        self.maze.setDirectionRouteBT(nextPos, parent)
                        stack.append((nextPos, parent))
                        moved = True
                        break 
            if not moved:
                if stack:
                    p_top, _ = stack[-1]
                    has_opts = False
                    for d in [Direction.South, Direction.East, Direction.West, Direction.North]:
                        if self.maze.canMove(p_top, d):
                            np = p_top.move(d)
                            if self.maze.getVisitOrder(np) == -1: 
                                has_opts = True
                                break
                    
                    if not has_opts:
                        p, _ = stack.pop()
                        self.maze.markOnStack(p, False)
                        
                        # === FIX: CLEAR GREY TRAIL ===
                        self.maze._clearFlag(p, InternalBit.VISITED_BIT.value)
                        
                        if self.maze.isJunction(p):
                            self.maze.markDeadJunction(p)

        if found:
            curr = end
            while True:
                self.maze.markPath(curr)
                yield "BACKTRACKING"
                if curr == start: break
                parent_dir = self.maze.getDirectionRouteBT(curr)
                if parent_dir == Direction.Uninitialized: break 
                curr = curr.move(parent_dir)
            yield "FINISHED"
        else:
            yield "NO_SOLUTION"


# --- MULTI-THREADED LOGIC (M2) ---

class DFSThread:
    def __init__(self, threadID, is_tb, start_pos, maze):
        self.id = threadID
        self.is_tb = is_tb
        self.stack = []
        self.maze = maze
        self.finished = False
        
        branches = Branches(maze, start_pos, threadID)
        self.stack.append(Junction(start_pos, Direction.Uninitialized, branches))
        
        self.state = 0 
        self.target_pos = None
        self.corridor_dir = Direction.Uninitialized
        self.backtrack_target = None 

    def step(self):
        if self.finished or not self.stack: return 'DEAD'
        
        # --- STATE 0: Junction Processing ---
        if self.state == 0: 
            junc = self.stack[-1]
            
            # Check collision BEFORE writing to map, otherwise we overwrite the evidence
            if self.is_tb:
                if junc.at == self.maze.getEnd(): return 'FOUND_TARGET'
                if self.maze.isVisitedByTeam(junc.at, False): return 'FOUND_TARGET' 
            else:
                if self.maze.isVisitedByTeam(junc.at, True): return 'FOUND_TARGET'
            
            self.maze.markVisitedTeam(junc.at, self.is_tb)
            self.maze.setThreadOwner(junc.at, self.id)
            
            d = Direction.Uninitialized
            if junc.branches.size() > 0:
                d = junc.branches.getNextThreads(junc.at, self.maze)
            
            if d == Direction.Uninitialized:
                self.stack.pop() 
                
                # Check for TRUE junction
                if self.maze.isJunction(junc.at):
                    self.maze.markDeadJunction(junc.at) 
                else:
                    self.maze.unmarkVisitedTeam(junc.at)
                
                if self.stack:
                    parent = self.stack[-1]
                    parent.branches.popCurrThreads(parent.at, self.maze)
                    
                    self.backtrack_target = parent.at
                    self.target_pos = junc.at 
                    self.state = 2 
                else:
                    return 'DEAD'
                return 'CONTINUE'
            
            self.state = 1
            self.corridor_dir = d
            self.target_pos = junc.at
            return 'CONTINUE'

        # --- STATE 1: Corridor Movement ---
        elif self.state == 1: 
            next_p = self.target_pos.move(self.corridor_dir)
            parent_rev = reverseDir(self.corridor_dir)
            
            # === CRITICAL FIX: Check Collision BEFORE Overwriting Ownership ===
            collision_found = False
            if self.is_tb:
                 if next_p == self.maze.getEnd() or self.maze.isVisitedByTeam(next_p, False):
                      collision_found = True
            else:
                 if self.maze.isVisitedByTeam(next_p, True):
                      collision_found = True
            
            if collision_found:
                # Do NOT setThreadOwner here. Leave the existing owner (the other team) 
                # so the solver can find the correct stack.
                self.target_pos = next_p
                # We do add a dummy junction to represent our arrival at the collision
                self.stack.append(Junction(next_p, parent_rev, Branches(self.maze, next_p, self.id)))
                return 'FOUND_TARGET'
            
            # No collision, safe to claim
            self.maze.markVisitedTeam(next_p, self.is_tb)
            self.maze.setThreadOwner(next_p, self.id)
            self.maze.setDirectionRouteBT(next_p, parent_rev)
            self.target_pos = next_p
            
            branches = Branches(self.maze, next_p, self.id)
            branches.remove(parent_rev) 
            
            if branches.size() != 1: 
                self.stack.append(Junction(next_p, parent_rev, branches))
                self.state = 0
            else:
                self.corridor_dir = branches.getNext()
            return 'CONTINUE'

        # --- STATE 2: Backtracking ---
        elif self.state == 2:
            if not self.maze.isJunction(self.target_pos):
                self.maze.unmarkVisitedTeam(self.target_pos)
            
            if self.target_pos == self.backtrack_target:
                self.state = 0
                return 'CONTINUE'
            
            parent_dir = self.maze.getDirectionRouteBT(self.target_pos)
            if parent_dir == Direction.Uninitialized:
                self.state = 0
                return 'CONTINUE'
                
            self.target_pos = self.target_pos.move(parent_dir)
            return 'CONTINUE'

        return 'DEAD'

class MTSolver:
    def __init__(self, maze: Maze):
        self.maze = maze
        self.threads = []

    def solve_step_by_step(self):
        start = self.maze.getStart()
        end = self.maze.getEnd()
        
        self.threads = []
        for i in range(3):
            self.threads.append(DFSThread(i, True, start, self.maze))
        for i in range(3):
            self.threads.append(DFSThread(i+3, False, end, self.maze))

        self.maze.markVisitedTeam(start, True)
        self.maze.setThreadOwner(start, 0)
        
        self.maze.markVisitedTeam(end, False)
        self.maze.setThreadOwner(end, 3)

        found = False
        collision_pos = None

        while not found:
            active_threads = 0
            for t in self.threads:
                if t.finished: continue
                
                res = t.step()
                if res == 'DEAD':
                    t.finished = True
                elif res == 'FOUND_TARGET':
                    found = True
                    if t.stack: 
                        collision_pos = t.target_pos 
                    break
                else:
                    active_threads += 1
            
            yield "SEARCHING"
            if active_threads == 0 and not found:
                break
        
        

        if found and collision_pos:
            # === PATH RECONSTRUCTION ===
            
            # --- PART 1: TOP-BOTTOM (Standard Backtracking) ---
            path_tb = []
            curr_tb = collision_pos
            
            # Safety: Ensure we are on a TB cell or neighbor
            if not self.maze.isVisitedByTeam(curr_tb, True):
                 for d in [Direction.North, Direction.East, Direction.South, Direction.West]:
                      n = curr_tb.move(d)
                      if self.maze.isVisitedByTeam(n, True): 
                          curr_tb = n
                          break
            
            temp = curr_tb
            while temp != start:
                path_tb.append(temp)
                parent_dir = self.maze.getDirectionRouteBT(temp)
                if parent_dir == Direction.Uninitialized: break
                temp = temp.move(parent_dir)
            path_tb.append(start)
            
            for p in reversed(path_tb): 
                self.maze.markPath(p)
                yield "BACKTRACKING"

            # --- PART 2: BOTTOM-TOP (Stack Based) ---
            
            # 1. Identify the BT Thread that owns the collision area
            bt_thread = None
            bt_start_curr = collision_pos
            
            # Try to find owner at collision or neighbors
            potential_positions = [collision_pos]
            for d in [Direction.North, Direction.East, Direction.South, Direction.West]:
                potential_positions.append(collision_pos.move(d))
            
            for pos in potential_positions:
                if self.maze.isVisitedByTeam(pos, False):
                    tid = self.maze.getThreadOwner(pos)
                    if tid >= 3:
                        bt_thread = self.threads[tid]
                        bt_start_curr = pos
                        break
            
            if bt_thread and bt_thread.stack:
                # We have the thread. Now we reconstruct using its STACK.
                # Logic: Walk from 'bt_start_curr' to 'End'.
                
                curr = bt_start_curr
                
                # --- FIXED: SEARCH FOR CORRECT STACK INDEX ---
                # We cannot assume stack[-1] is our location. We must search for 'curr'.
                stack_idx = -1
                for i in range(len(bt_thread.stack)):
                    if bt_thread.stack[i].at == curr:
                        stack_idx = i
                        break
                
                # If not found (e.g. collision on a corridor cell, not a junction),
                # we must find the closest future junction in the stack.
                # However, for simplicity in corridors, we rely on the nearest *past* stack node
                # to determine direction, but we are moving towards END.
                # The stack grows End -> Collision. So 'past' index means closer to collision.
                # If we are in a corridor, the stack tip (or near it) should be our guide.
                
                if stack_idx == -1:
                    # Fallback: Start from the top of the stack (nearest to collision)
                    stack_idx = len(bt_thread.stack) - 1
                
                came_from_dir = Direction.Uninitialized
                
                while curr != end:
                    self.maze.markPath(curr)
                    yield "BACKTRACKING"
                    
                    go_to = Direction.Uninitialized
                    
                    # A. Check Stack Synchronization
                    # If our current position is the node recorded in the stack:
                    if stack_idx >= 0 and curr == bt_thread.stack[stack_idx].at:
                        # Follow the instruction recorded in history
                        go_to = bt_thread.stack[stack_idx].came_from
                        stack_idx -= 1
                    
                    # B. Corridor Logic
                    else:
                        branches = Branches(self.maze, curr, 0)
                        if came_from_dir != Direction.Uninitialized:
                            branches.remove(came_from_dir)
                        
                        if branches.size() == 1:
                            go_to = branches.getNext()
                        else:
                            # Fallback/Error state
                            break

                    if go_to == Direction.Uninitialized: break
                    
                    curr = curr.move(go_to)
                    came_from_dir = reverseDir(go_to)
                
                self.maze.markPath(end)
            else:
                print("Error: Could not find valid BT Thread stack for reconstruction.")

            yield "FINISHED"
        else:
            yield "NO_SOLUTION"

# ==========================================
# 6. MT_M1 SOLVER
# ==========================================

class PruneThread:
    def __init__(self, tid, maze, row_start, row_end, in_q, out_qs):
        self.id = tid
        self.maze = maze
        self.row_start = row_start
        self.row_end = row_end
        self.in_q = in_q
        self.out_qs = out_qs # List of output queues [Top, Bottom] or just neighbors
        self.stack = []
        self.phase = 'SCAN'
        self.iter_row = row_start
        self.iter_col = 0
        self.finished = False

    def step(self):
        if self.finished: return

        # 1. SCAN PHASE
        if self.phase == 'SCAN':
            # Scan a chunk of cells per frame to avoid lag, but here we do it incrementally
            # To visualize slowly, we do one row or a few cells per step.
            # Let's do one row per step to be visible
            for _ in range(self.maze.width):
                if self.iter_col >= self.maze.width:
                    self.iter_col = 0
                    self.iter_row += 1
                
                if self.iter_row >= self.row_end:
                    self.phase = 'PRUNE'
                    return

                pos = Position(self.iter_row, self.iter_col)
                # Skip Start/End
                if pos == self.maze.getStart() or pos == self.maze.getEnd():
                    self.iter_col += 1
                    continue

                moves = self.maze.getAvailableMovesNoPruned(pos)
                if len(moves) <= 1:
                    self.stack.append(pos)
                
                self.iter_col += 1

        # 2. PRUNE PHASE
        elif self.phase == 'PRUNE':
            
            # Process incoming queue from neighbors
            while self.in_q:
                p_pos = self.in_q.popleft()
                self.stack.append(p_pos)

            if not self.stack:
                # No work, but keep alive waiting for neighbors
                return 

            # Process stack (do a few per frame)
            # C++ logic: while (!stackDFS.empty())
            processed_count = 0
            # === MODIFICATION HERE: Change 5 to 1 for gradual single-step effect ===
            while self.stack and processed_count < 1: 
                pos = self.stack.pop()
                processed_count += 1

                if self.maze.isPruned(pos): continue
                
                self.maze.markPruned(pos)
                self.maze.setThreadOwner(pos, self.id) # For coloring

                # Get moves ignoring currently pruned cells
                moves = self.maze.getAvailableMovesNoPruned(pos)
                
                if len(moves) == 1:
                    d = moves[0]
                    neighbor = pos.move(d)

                    if neighbor == self.maze.getStart() or neighbor == self.maze.getEnd():
                        continue

                    n_moves = self.maze.getAvailableMovesNoPruned(neighbor)
                    
                    # Logic says: if neighbors moves <= 1 (after pruning current), it becomes dead
                    # In our case, getAvailableMovesNoPruned(neighbor) sees 'pos' as NOT pruned yet 
                    # because we just marked it? No, we marked it above.
                    # So n_moves does NOT include 'pos'.
                    # So if n_moves <= 1, neighbor is dead.
                    
                    if len(n_moves) <= 1:
                        # Check boundary
                        if neighbor.row < self.row_start:
                            # Send to Top Neighbor
                            if self.out_qs[0] is not None: self.out_qs[0].append(neighbor)
                        elif neighbor.row >= self.row_end:
                            # Send to Bottom Neighbor
                            if self.out_qs[1] is not None: self.out_qs[1].append(neighbor)
                        else:
                            self.stack.append(neighbor)

class WalkThreadTB:
    def __init__(self, maze, solve_list):
        self.maze = maze
        self.solve_list = solve_list # Shared list to store result
        self.curr = maze.getStart()
        self.came_from = Direction.Uninitialized
        self.finished = False
        self.found = False
        self.overlap = None

    def step(self, first_exit_ref):
        if self.finished: return

        target = self.maze.getEnd()
        
        # Check if BT thread reached here or we are overlapping
        if self.maze.getDirectionRouteBT(self.curr) != Direction.Uninitialized:
            self.overlap = self.curr
            self.finished = True
            return

        if self.curr == target:
            self.found = True
            first_exit_ref[0] = True
            self.finished = True
            return

        moves = self.maze.getAvailableMovesNoPruned(self.curr)
        
        # Remove came_from
        if self.came_from != Direction.Uninitialized:
            if self.came_from in moves:
                moves.remove(self.came_from)
        
        go_to = Direction.Uninitialized
        
        if len(moves) == 1:
            go_to = moves[0]
            self.solve_list.append(go_to)
            self.curr = self.curr.move(go_to)
            self.overlap = self.curr
            self.came_from = reverseDir(go_to)
            
            # Visual marker
            self.maze.markVisitedTeam(self.curr, True) 
            
        elif len(moves) > 1:
            # Wait for pruning to reduce choices
            return 
        elif len(moves) == 0:
            # Dead end (shouldn't happen if pruning is correct)
            self.finished = True

class BFSThreadBT:
    def __init__(self, maze):
        self.maze = maze
        self.q = deque([maze.getEnd()])
        self.maze.setDirectionRouteBT(maze.getEnd(), Direction.Uninitialized)
        self.finished = False
        self.found = False

    def step(self, first_exit_ref):
        if self.finished or first_exit_ref[0]: 
            self.finished = True
            return

        end_node = self.maze.getStart()
        
        # Process a few nodes per frame
        steps = 0
        while self.q and steps < 2:
            steps += 1
            cur = self.q.popleft()

            if self.maze.isPruned(cur): continue

            if cur == end_node:
                first_exit_ref[0] = True
                self.found = True
                self.finished = True
                return

            came_from = self.maze.getDirectionRouteBT(cur)

            for d in [Direction.South, Direction.West, Direction.East, Direction.North]:
                # C++ Logic: "if came_from != d && canMove(d)"
                # But d here is the direction TO neighbor. came_from is direction TO parent.
                # Actually C++ says: if (came_from != Direction::South && pMaze->canMove(cur, Direction::South))
                # meaning if we didn't come FROM south (parent is south), we can go south.
                
                # Check valid move
                if self.maze.canMove(cur, d):
                    nextPos = cur.move(d)
                    
                    if self.maze.isPruned(nextPos): continue

                    if self.maze.getDirectionRouteBT(nextPos) == Direction.Uninitialized:
                        # Mark parent
                        parent = reverseDir(d)
                        self.maze.setDirectionRouteBT(nextPos, parent)
                        self.q.append(nextPos)
                        
                        # Visual
                        self.maze.markVisitedTeam(nextPos, False)

class MT_M1_Solver:
    def __init__(self, maze: Maze):
        self.maze = maze
        self.pruners = []
        self.walker = None
        self.bfs = None
        self.solve_list = []
        self.first_exit = [False] # Reference wrapper
        
        # Setup Pruners
        N = 4
        chunk = maze.height // N
        remainder = maze.height % N
        
        queues = [deque() for _ in range(N)] # In-queues for each thread
        
        for i in range(N):
            row_start = i * chunk + min(i, remainder)
            row_end = (i + 1) * chunk + min(i + 1, remainder)
            
            # Neighbors queues: [Top, Bottom]
            out_qs = [None, None]
            if i > 0: out_qs[0] = queues[i-1]
            if i < N-1: out_qs[1] = queues[i+1]
            
            self.pruners.append(PruneThread(i, maze, row_start, row_end, queues[i], out_qs))
            
        self.walker = WalkThreadTB(maze, self.solve_list)
        self.bfs = BFSThreadBT(maze)

    def solve_step_by_step(self):
        
        # Main Loop: Runs as long as solution isn't found and searchers are active
        while not self.first_exit[0] and (not self.walker.finished or not self.bfs.finished):
            
            # 1. Step Pruners (Concurrent)
            for p in self.pruners:
                p.step()
            
            # 2. Step Walker (Concurrent)
            self.walker.step(self.first_exit)
            
            # 3. Step BFS (Concurrent)
            self.bfs.step(self.first_exit)
            
            if self.walker.finished and self.walker.overlap:
                 break
                 
            yield "SEARCHING"

        # Reconstruction
        # 1. TB Path part
        curr = self.maze.getStart()
        # Draw TB part
        for d in self.solve_list:
            self.maze.markPath(curr)
            curr = curr.move(d)
            yield "BACKTRACKING"
        
        # Draw remaining from Overlap to End using BT hints
        while curr != self.maze.getEnd():
            self.maze.markPath(curr)
            d = self.maze.getDirectionRouteBT(curr)
            if d == Direction.Uninitialized: break
            curr = curr.move(d)
            yield "BACKTRACKING"
            
        self.maze.markPath(self.maze.getEnd())
        yield "FINISHED"

# ==========================================
# 7. MAIN
# ==========================================

def main():
    MAZE_FILE = "Maze_Data/Maze50x50.data"
    file_name = os.path.basename(MAZE_FILE)
    
    # --- CONFIG ---
    CELL_W = 21
    CELL_H = 16 
    WALL_THICKNESS = 2
    FPS = 60
    CONTROL_PANEL_WIDTH = 250 

    COLOR_BG = (255, 255, 255)
    COLOR_WALL = (0, 0, 0)
    
    # Updated: Deep Taupe
    COLOR_DEAD = (183, 183, 164)
    
    COLOR_VISITED = (220, 220, 220) 
    COLOR_BFS_VISITED = (255, 215, 120) 

    # Updated: Electric Lime
    COLOR_PATH = (0, 100, 0)        
    
    COLOR_DFS_PATH = (100, 149, 237) 
    COLOR_JUNCTION = (255, 185, 0)       
    
    # MT_M2 Thread Colors
    THREAD_COLORS = {
        0: (255, 120, 120), 
        1: (220, 60, 60),    
        2: (180, 0, 0),      
        3: (120, 200, 255), 
        4: (60, 140, 220),  
        5: (0, 0, 180)      
    }

    # MT_M1 Pruning Colors (Darker shades for pruned areas)
    PRUNE_COLORS = {
        0: (255, 192, 198),   # Light Pink
        1: (240, 164, 144),   # Light Yellow
        2: (147, 238, 147),   # Light Green
        3: (173, 200, 230)    # Light Blue
    }

    COLOR_TEXT_NORMAL = (50, 50, 50) 
    COLOR_TEXT_WHITE = (255, 255, 255)
    
    maze = Maze()
    try:
        maze.Load(MAZE_FILE)
    except FileNotFoundError:
        print(f"Error: File {MAZE_FILE} not found.")
        return

    pygame.init()
    
    maze_w_px = maze.width * CELL_W
    maze_h_px = maze.height * CELL_H
    screen_w = maze_w_px + CONTROL_PANEL_WIDTH
    screen_h = maze_h_px
    screen = pygame.display.set_mode((screen_w, screen_h))
    
    pygame.display.set_caption(f"Maze Solver - {file_name}")
    
    clock = pygame.time.Clock()
    font = pygame.font.SysFont('Arial', 18)
    font_cell = pygame.font.SysFont('Arial', 10, bold=True) # Slightly larger/bold for visibility

    app_state = {
        'algorithm': 'BFS', 
        'generator': None,
        'state': "RUNNING",
        'solver': None
    }

    def init_solver():
        maze.Reset()
        if app_state['algorithm'] == 'BFS':
            app_state['solver'] = BFSSolver(maze)
        elif app_state['algorithm'] == 'DFS':
            app_state['solver'] = DFSSolver(maze)
        elif app_state['algorithm'] == 'MT_M2':
            app_state['solver'] = MTSolver(maze)
        elif app_state['algorithm'] == 'MT_M1':
            app_state['solver'] = MT_M1_Solver(maze)
            
        app_state['generator'] = app_state['solver'].solve_step_by_step()
        app_state['state'] = "RUNNING"
        
        # Update Title
        pygame.display.set_caption(f"Maze Solver - {file_name} [{app_state['algorithm']}]")

    init_solver()

    def set_bfs():
        app_state['algorithm'] = 'BFS'
        btn_bfs.is_active = True
        btn_dfs.is_active = False
        btn_mt.is_active = False
        btn_mt1.is_active = False
        init_solver()

    def set_dfs():
        app_state['algorithm'] = 'DFS'
        btn_bfs.is_active = False
        btn_dfs.is_active = True
        btn_mt.is_active = False
        btn_mt1.is_active = False
        init_solver()
        
    def set_mt():
        app_state['algorithm'] = 'MT_M2'
        btn_bfs.is_active = False
        btn_dfs.is_active = False
        btn_mt.is_active = True
        btn_mt1.is_active = False
        init_solver()
    
    def set_mt1():
        app_state['algorithm'] = 'MT_M1'
        btn_bfs.is_active = False
        btn_dfs.is_active = False
        btn_mt.is_active = False
        btn_mt1.is_active = True
        init_solver()

    def restart():
        init_solver()

    # UI setup
    panel_start_x = maze_w_px + 20
    slider = SimpleSlider(panel_start_x, 80, 200, 20, -10, 20, 0)
    
    btn_bfs = SimpleButton(panel_start_x, 150, 65, 30, "BFS", set_bfs, color=(100, 100, 200))
    btn_dfs = SimpleButton(panel_start_x + 70, 150, 65, 30, "DFS", set_dfs, color=(100, 100, 200))
    btn_mt  = SimpleButton(panel_start_x + 140, 150, 65, 30, "MT_M2", set_mt, color=(100, 100, 200))
    
    # New MT_M1 Button (Right Side Toggle)
    btn_mt1 = SimpleButton(panel_start_x + 140, 190, 65, 30, "MT_M1", set_mt1, color=(100, 150, 150))
    
    btn_restart = SimpleButton(panel_start_x + 50, 240, 100, 30, "Restart", restart)

    btn_bfs.is_active = True

    frame_counter = 0
    running = True

    while running:
        events = pygame.event.get()
        for event in events:
            if event.type == pygame.QUIT:
                running = False
            
            slider.handle_event(event)
            btn_bfs.handle_event(event)
            btn_dfs.handle_event(event)
            btn_mt.handle_event(event)
            btn_mt1.handle_event(event)
            btn_restart.handle_event(event)

        val = int(slider.get_value())
        current_gen = app_state['generator']
        current_state = app_state['state']

        if current_state not in ["FINISHED", "NO_SOLUTION"]:
            if val < 0:
                delay = abs(val)
                frame_counter += 1
                if frame_counter > delay:
                    frame_counter = 0
                    try:
                        app_state['state'] = next(current_gen)
                    except StopIteration:
                        app_state['state'] = "FINISHED"
            else:
                steps = val + 1
                for _ in range(steps):
                    try:
                        app_state['state'] = next(current_gen)
                        if app_state['state'] in ["FINISHED", "NO_SOLUTION"]: break
                    except StopIteration:
                        app_state['state'] = "FINISHED"
                        break

        # --- DRAWING ---
        screen.fill(COLOR_BG)

        for r in range(maze.height):
            for c in range(maze.width):
                pos = Position(r, c)
                val = maze.getCell(pos)
                if val == 0: continue
                
                x, y = c * CELL_W, r * CELL_H
                rect = (x, y, CELL_W, CELL_H)
                
                is_path = (val & InternalBit.PATH_BIT.value)
                is_visited = (val & InternalBit.VISITED_BIT.value)
                on_stack = (val & InternalBit.ON_STACK_BIT.value)
                is_dead_junction = (val & InternalBit.DEAD_JUNCTION_BIT.value)
                is_pruned = (val & InternalBit.PRUNED_BIT.value)
                
                # 1. Final Path (Highest Priority)
                if is_path:
                    pygame.draw.rect(screen, COLOR_PATH, rect)

                # 2. MT_M1 Pruned Cells (Second Highest - Overrides Junctions)
                elif app_state['algorithm'] == 'MT_M1' and is_pruned:
                    tid = maze.getThreadOwner(pos)
                    t_color = PRUNE_COLORS.get(tid, COLOR_DEAD)
                    pygame.draw.rect(screen, t_color, rect)
                    
                    # --- UPDATED: Show Thread Num instead of face ---
                    if tid != -1:
                        txt_surf = font_cell.render(str(tid), True, (50, 50, 50))
                        txt_rect = txt_surf.get_rect(center=(x + CELL_W//2, y + CELL_H//2))
                        screen.blit(txt_surf, txt_rect)
                
                # 3. Dead Junctions (Dark Gray)
                elif is_dead_junction:
                    pygame.draw.rect(screen, COLOR_DEAD, rect)
                
                # 4. Orange Junctions (Persistent Active)
                elif maze.isJunction(pos) and (is_visited or on_stack or (val & (InternalBit.VISITED_TB.value | InternalBit.VISITED_BT.value))):
                    pygame.draw.rect(screen, COLOR_JUNCTION, rect)
                
                # 5. MT_M1 Remaining Rendering (Walker, BFS)
                elif app_state['algorithm'] == 'MT_M1':
                    # Pruned cells already handled above
                    if (val & InternalBit.VISITED_TB.value):
                          # Walker TB
                          pygame.draw.rect(screen, (50, 205, 50), rect) # Lime Green
                    elif (val & InternalBit.VISITED_BT.value): # Actually re-using VISITED_BT bit for BFS BT
                          # BFS BT
                          pygame.draw.rect(screen, (255, 215, 0), rect) # Gold
                    elif (val & InternalBit.VISITED_BIT.value):
                          pygame.draw.rect(screen, COLOR_VISITED, rect)

                # 6. MT_M2 Rendering (Thread Colors)
                elif app_state['algorithm'] == 'MT_M2':
                    tid = maze.getThreadOwner(pos)
                    
                    # A. Draw Background Colors (Rect Only)
                    if (val & InternalBit.VISITED_TB.value) or (val & InternalBit.VISITED_BT.value):
                        t_color = THREAD_COLORS.get(tid, (150, 150, 150))
                        pygame.draw.rect(screen, t_color, rect)
                    elif (val & InternalBit.VISITED_BIT.value):
                        pygame.draw.rect(screen, COLOR_VISITED, rect)
                            
                # 7. DFS Rendering
                elif app_state['algorithm'] == 'DFS':
                    if on_stack:
                        pygame.draw.rect(screen, COLOR_DFS_PATH, rect)
                    elif is_visited:
                        pygame.draw.rect(screen, COLOR_VISITED, rect)

                # 8. BFS Rendering
                elif is_visited:
                    color = COLOR_BFS_VISITED if app_state['algorithm'] == 'BFS' else COLOR_VISITED
                    pygame.draw.rect(screen, color, rect)
                
                # --- MT_M2 Thread Number Overlay (Correctly placed AFTER background Rects) ---
                if app_state['algorithm'] == 'MT_M2':
                    tid = maze.getThreadOwner(pos)
                    if tid != -1:
                        # Determine text color based on background
                        txt_color = (0, 0, 0)
                        
                        # 1. Path (Green)
                        if is_path:
                            txt_color = (255, 255, 255)
                        # 2. Dead Junction (Dark Gray)
                        elif is_dead_junction:
                            txt_color = (255, 255, 255)
                        # 3. Active Junction (Orange)
                        elif maze.isJunction(pos) and (is_visited or on_stack or (val & (InternalBit.VISITED_TB.value | InternalBit.VISITED_BT.value))):
                            # Orange background (255, 185, 0) -> Black text
                            txt_color = (0, 0, 0) 
                        # 4. Standard Visited (Thread Color)
                        else:
                            # Use White for Darker threads (2, 5)
                            if tid in [2, 5]: 
                                txt_color = (255, 255, 255)

                        txt_surf = font_cell.render(str(tid), True, txt_color)
                        txt_rect = txt_surf.get_rect(center=(x + CELL_W//2, y + CELL_H//2))
                        screen.blit(txt_surf, txt_rect)
                
                # Order Numbers (ONLY if NOT MT)
                if app_state['algorithm'] not in ['MT_M2', 'MT_M1']:
                    order = maze.getVisitOrder(pos)
                    if order != -1:
                        is_dark_bg = is_path or (on_stack and app_state['algorithm'] == 'DFS') or is_dead_junction
                        txt_color = COLOR_TEXT_WHITE if is_dark_bg else COLOR_TEXT_NORMAL
                        txt_surf = font_cell.render(str(order), True, txt_color)
                        txt_rect = txt_surf.get_rect(center=(x + CELL_W//2, y + CELL_H//2))
                        screen.blit(txt_surf, txt_rect)

        # Walls
        for r in range(maze.height):
            for c in range(maze.width):
                pos = Position(r, c)
                val = maze.getCell(pos)
                x, y = c * CELL_W, r * CELL_H
                if (val & InternalBit.EAST_BIT.value):
                    pygame.draw.line(screen, COLOR_WALL, (x+CELL_W, y), (x+CELL_W, y+CELL_H), WALL_THICKNESS)
                if (val & InternalBit.SOUTH_BIT.value):
                    pygame.draw.line(screen, COLOR_WALL, (x, y+CELL_H), (x+CELL_W, y+CELL_H), WALL_THICKNESS)

        # === 9. Start & End Arrows ===
        start_pos = maze.getStart()
        end_pos = maze.getEnd()
        
        # Helper to draw simple arrow
        def draw_map_arrow(pos, color):
            x, y = pos.col * CELL_W, pos.row * CELL_H
            # Points for a downward triangle arrow
            points = [
                (x + 3, y + 3),
                (x + CELL_W - 3, y + 3),
                (x + CELL_W // 2, y + CELL_H - 3)
            ]
            pygame.draw.polygon(screen, color, points)
            pygame.draw.polygon(screen, (0, 0, 0), points, 1) # Outline

        draw_map_arrow(start_pos, (0, 255, 0)) # Green Arrow for Start
        draw_map_arrow(end_pos, (255, 0, 0))   # Red Arrow for End

        # Control Panel
        pygame.draw.rect(screen, (230, 230, 230), (maze_w_px, 0, CONTROL_PANEL_WIDTH, screen_h))
        pygame.draw.line(screen, (100, 100, 100), (maze_w_px, 0), (maze_w_px, screen_h), 2)
        
        txt_algo = font.render(f"Mode: {app_state['algorithm']}", True, (0, 0, 0))
        screen.blit(txt_algo, (panel_start_x, 20))

        txt_speed = font.render("Speed:", True, (0, 0, 0))
        screen.blit(txt_speed, (panel_start_x, 60))
        
        slider.draw(screen)
        btn_bfs.draw(screen)
        btn_dfs.draw(screen)
        btn_mt.draw(screen)
        btn_mt1.draw(screen)
        btn_restart.draw(screen)
        
        # === LEGEND RENDERER (Updated) ===
        legend_y = 300
        
        # 1. Determine items based on algorithm
        legend_items = []
        
        if app_state['algorithm'] == 'MT_M2':
            # MT Mode: Threads First
            title = font.render("Thread Legend:", True, (0, 0, 0))
            screen.blit(title, (panel_start_x, legend_y))
            legend_y += 30
            for tid, color in THREAD_COLORS.items():
                legend_items.append((color, f"{'TB' if tid < 3 else 'BT'} Thread {tid}"))
            
            # Add separation
            legend_items.append(None) 
        
        elif app_state['algorithm'] == 'MT_M1':
            title = font.render("Pruning Legend:", True, (0, 0, 0))
            screen.blit(title, (panel_start_x, legend_y))
            legend_y += 30
            for tid, color in PRUNE_COLORS.items():
                legend_items.append((color, f"Pruner Chunk {tid}"))
            legend_items.append(None)
            legend_items.append(((50, 205, 50), "TB Walker"))
            legend_items.append(((255, 215, 0), "BT BFS Search"))
            
        elif app_state['algorithm'] == 'DFS':
            # DFS Mode: Stack First
            title = font.render("DFS Legend:", True, (0, 0, 0))
            screen.blit(title, (panel_start_x, legend_y))
            legend_y += 30
            legend_items.append((COLOR_DFS_PATH, "Search Path"))
            legend_items.append(None)

        # 2. Add Common Junction Items (Active & Dead)
        if app_state['algorithm'] in ['MT_M2', 'DFS', 'MT_M1']:
            legend_items.append((COLOR_JUNCTION, "Active Junc"))
            legend_items.append((COLOR_DEAD, "Dead Junc"))
            # Also show final path color
            legend_items.append((COLOR_PATH, "Final Path"))

        # 3. Render Items
        for item in legend_items:
            if item is None:
                legend_y += 10 # Spacer
                continue
                
            color, text = item
            # Color Box
            pygame.draw.rect(screen, color, (panel_start_x, legend_y, 20, 20), border_radius=3)
            pygame.draw.rect(screen, (0, 0, 0), (panel_start_x, legend_y, 20, 20), 1, border_radius=3)
            
            # Text
            txt_surf = font.render(text, True, (50, 50, 50))
            screen.blit(txt_surf, (panel_start_x + 30, legend_y))
            
            legend_y += 25

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()