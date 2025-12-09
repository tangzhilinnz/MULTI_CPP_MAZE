import struct
import pygame
import sys
from collections import deque
from enum import Enum, auto

# ==========================================
# 1. ENUMS & CONSTANTS
# ==========================================

class Direction(Enum):
    North = auto()
    East = auto()
    South = auto()
    West = auto()
    Uninitialized = auto()

class InternalBit(Enum):
    EAST_BIT = 1
    SOUTH_BIT = 2
    VISITED_BIT = 4    
    PATH_BIT = 8       
    
    # Backtracking bits
    PARENT_NORTH = 16
    PARENT_EAST  = 32
    PARENT_SOUTH = 64
    PARENT_WEST  = 128
    PARENT_MASK  = 240 

# ==========================================
# 2. HELPER CLASSES
# ==========================================

class Position:
    def __init__(self, row=0, col=0):
        self.row = row
        self.col = col
    def __eq__(self, other): return self.row == other.row and self.col == other.col
    def move(self, dir: Direction):
        if dir == Direction.North: return Position(self.row - 1, self.col)
        if dir == Direction.South: return Position(self.row + 1, self.col)
        if dir == Direction.East:  return Position(self.row, self.col + 1)
        if dir == Direction.West:  return Position(self.row, self.col - 1)
        return self
    def __repr__(self): return f"({self.row}, {self.col})"

# ==========================================
# 3. UI ELEMENTS: SLIDER & BUTTON
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
        elif event.type == pygame.MOUSEMOTION:
            if self.dragging:
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

    def get_value(self):
        return self.val


class SimpleButton:
    def __init__(self, x, y, w, h, text, callback):
        self.rect = pygame.Rect(x, y, w, h)
        self.text = text
        self.callback = callback
        self.font = pygame.font.SysFont('Arial', 18, bold=True)
        self.bg_color = (200, 50, 50)
        self.hover_color = (255, 80, 80)
        self.is_hovered = False

    def handle_event(self, event):
        if event.type == pygame.MOUSEMOTION:
            self.is_hovered = self.rect.collidepoint(event.pos)
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if self.rect.collidepoint(event.pos):
                self.callback()

    def draw(self, screen):
        color = self.hover_color if self.is_hovered else self.bg_color
        pygame.draw.rect(screen, color, self.rect, border_radius=5)
        pygame.draw.rect(screen, (0, 0, 0), self.rect, 2, border_radius=5)
        
        text_surf = self.font.render(self.text, True, (255, 255, 255))
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)

# ==========================================
# 4. MAZE & SOLVER
# ==========================================

class Maze:
    def __init__(self):
        self.width = 0
        self.height = 0
        self.poMazeData = []

    def Load(self, filename):
        with open(filename, 'rb') as f:
            header_bytes = f.read(12)
            self.width, self.height, solvable = struct.unpack('iii', header_bytes)
            self.poMazeData = [0] * (self.width * self.height)
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
        """Clears Visited, Path, and Parent bits, keeping Walls intact."""
        mask = ~(InternalBit.VISITED_BIT.value | 
                 InternalBit.PATH_BIT.value | 
                 InternalBit.PARENT_MASK.value)
        
        for i in range(len(self.poMazeData)):
            self.poMazeData[i] &= mask

    def getStart(self) -> Position:
        return Position(0, self.width // 2)
    def getEnd(self) -> Position:
        return Position(self.height - 1, self.width // 2)
    def _cellIndex(self, pos: Position) -> int:
        return pos.row * self.width + pos.col
    def getCell(self, pos: Position) -> int:
        if 0 <= pos.row < self.height and 0 <= pos.col < self.width:
            return self.poMazeData[self._cellIndex(pos)]
        return 0
    def setCell(self, pos: Position, value: int):
        self.poMazeData[self._cellIndex(pos)] = value
    def _hasFlag(self, pos: Position, val) -> bool:
        return (self.getCell(pos) & val) != 0
    def _setFlag(self, pos: Position, val):
        self.setCell(pos, self.getCell(pos) | val)

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
        if not (val & InternalBit.VISITED_BIT.value): return Direction.Uninitialized
        if val & InternalBit.PARENT_NORTH.value: return Direction.North
        if val & InternalBit.PARENT_EAST.value: return Direction.East
        if val & InternalBit.PARENT_SOUTH.value: return Direction.South
        if val & InternalBit.PARENT_WEST.value: return Direction.West
        return Direction.Uninitialized

    def markPath(self, pos: Position):
        self._setFlag(pos, InternalBit.PATH_BIT.value)


class BFSSolver:
    def __init__(self, maze: Maze):
        self.maze = maze

    def solve_step_by_step(self):
        start = self.maze.getStart()
        end = self.maze.getEnd()
        q = deque()
        q.append(start)
        self.maze.setDirectionRouteBT(start, Direction.Uninitialized)
        found = False

        # Phase 1: Search
        while q:
            cur = q.popleft()
            yield "SEARCHING"
            if cur == end:
                found = True
                break
            
            for d in [Direction.South, Direction.West, Direction.East, Direction.North]:
                if self.maze.canMove(cur, d):
                    nextPos = cur.move(d)
                    if self.maze.getDirectionRouteBT(nextPos) == Direction.Uninitialized:
                        # Reverse direction to store "Parent"
                        parent = Direction.Uninitialized
                        if d == Direction.North: parent = Direction.South
                        elif d == Direction.South: parent = Direction.North
                        elif d == Direction.East: parent = Direction.West
                        elif d == Direction.West: parent = Direction.East
                        
                        self.maze.setDirectionRouteBT(nextPos, parent)
                        q.append(nextPos)

        # Phase 2: Backtrack
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

# ==========================================
# 5. MAIN
# ==========================================

def main():
    MAZE_FILE = "Maze_Data/Maze50x50.data"
    CELL_SIZE = 15
    WALL_THICKNESS = 2
    FPS = 60
    CONTROL_PANEL_HEIGHT = 60

    COLOR_BG = (255, 255, 255)
    COLOR_WALL = (0, 0, 0)
    COLOR_VISITED = (152, 251, 152) 
    COLOR_PATH = (34, 139, 34)
    
    maze = Maze()
    try:
        maze.Load(MAZE_FILE)
    except FileNotFoundError:
        print(f"Error: File {MAZE_FILE} not found.")
        return

    pygame.init()
    
    maze_w_px = maze.width * CELL_SIZE
    maze_h_px = maze.height * CELL_SIZE
    screen = pygame.display.set_mode((maze_w_px, maze_h_px + CONTROL_PANEL_HEIGHT))
    
    pygame.display.set_caption("BFS Algorithm")
    
    clock = pygame.time.Clock()
    font = pygame.font.SysFont('Arial', 18)

    # --- STATE MANAGEMENT ---
    # We wrap creation in a dictionary so the button callback can modify it
    app_state = {
        'solver': BFSSolver(maze),
        'generator': None,
        'state': "RUNNING"
    }
    app_state['generator'] = app_state['solver'].solve_step_by_step()

    # --- RESTART CALLBACK ---
    def restart_game():
        maze.Reset()
        app_state['solver'] = BFSSolver(maze)
        app_state['generator'] = app_state['solver'].solve_step_by_step()
        app_state['state'] = "RUNNING"

    # --- UI INIT ---
    # Slider: x=100, width=300
    slider = SimpleSlider(100, maze_h_px + 20, 300, 20, -10, 20, 0)
    
    # Button: Right of slider (x=420)
    btn_restart = SimpleButton(420, maze_h_px + 15, 100, 30, "Restart", restart_game)

    frame_counter = 0

    running = True
    while running:
        events = pygame.event.get()
        for event in events:
            if event.type == pygame.QUIT:
                running = False
            
            slider.handle_event(event)
            btn_restart.handle_event(event)

        # Speed Logic
        val = int(slider.get_value())
        
        # Access current state via dictionary
        current_gen = app_state['generator']
        current_state = app_state['state']

        if current_state != "FINISHED" and current_state != "NO_SOLUTION":
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
                x, y = c * CELL_SIZE, r * CELL_SIZE
                rect = (x, y, CELL_SIZE, CELL_SIZE)
                if (val & InternalBit.PATH_BIT.value):
                    pygame.draw.rect(screen, COLOR_PATH, rect)
                elif (val & InternalBit.VISITED_BIT.value):
                    pygame.draw.rect(screen, COLOR_VISITED, rect)

        for r in range(maze.height):
            for c in range(maze.width):
                pos = Position(r, c)
                val = maze.getCell(pos)
                x, y = c * CELL_SIZE, r * CELL_SIZE
                if (val & InternalBit.EAST_BIT.value):
                    pygame.draw.line(screen, COLOR_WALL, (x+CELL_SIZE, y), (x+CELL_SIZE, y+CELL_SIZE), WALL_THICKNESS)
                if (val & InternalBit.SOUTH_BIT.value):
                    pygame.draw.line(screen, COLOR_WALL, (x, y+CELL_SIZE), (x+CELL_SIZE, y+CELL_SIZE), WALL_THICKNESS)

        # Draw Control Panel
        panel_y = maze_h_px
        pygame.draw.rect(screen, (230, 230, 230), (0, panel_y, maze_w_px, CONTROL_PANEL_HEIGHT))
        pygame.draw.line(screen, (100, 100, 100), (0, panel_y), (maze_w_px, panel_y), 1)
        
        txt_speed = font.render("Speed:", True, (0, 0, 0))
        screen.blit(txt_speed, (20, panel_y + 20))
        
        # Slider & Button
        slider.draw(screen)
        btn_restart.draw(screen)

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()