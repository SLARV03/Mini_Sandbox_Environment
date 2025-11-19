
#!/usr/bin/env python3
"""
finaltui2_fixed.py
Patched TUI: shows stats for all sandbox-related processes and logs new/ended processes.

How it detects sandbox processes:
 - For each system process, inspect proc.cmdline() and proc.cwd()
 - If "sandbox_env" appears in either, treat it as sandbox-related
 - This avoids relying on PID 1 or sandbox_cli parent-child relationship

Controls:
  R  - Launch sandbox terminal (attempts to open Windows Terminal via wt.exe)
  M  - Cycle mode (OPEN -> RESTRICTED -> LOCKED)
  E  - Edit limits (UI-only)
  Q  - Quit
  ↑/↓ - Scroll logs
"""

import curses
import time
import subprocess
import os
from collections import deque

import psutil

# ---------- CONFIG ----------
PROJECT_DIR = "/mnt/d/pbl_os/Mini_Sandbox_Environment"
SANDBOX_RUN = os.path.join(PROJECT_DIR, "scripts", "run_sandbox.sh")
LOG_FILE = os.path.join(PROJECT_DIR, "sandbox_activity.log")

LOG_HISTORY = 2000
log_lines = deque(maxlen=LOG_HISTORY)

# keep track of processes we have seen (pid -> cmd)
prev_seen = {}

# keep track of first seen CPU seconds for CPUsec delta calc
process_cpu_start = {}

scroll = 0

status = "STOPPED"
sandbox_pid = None
sandbox_proc = None
start_time = None

# ---------- Modes & Defaults ----------
MODES = ["OPEN", "RESTRICTED", "LOCKED"]
mode_index = 1  # default to RESTRICTED

DEFAULT_LIMITS = {
    "OPEN": {
        "cpu_time": None,
        "ram_mb": None,
        "nproc": None,
        "nofile": None
    },
    "RESTRICTED": {
        "cpu_time": 3,
        "ram_mb": 256,
        "nproc": 10,
        "nofile": 32
    },
    "LOCKED": {
        "cpu_time": 1,
        "ram_mb": 128,
        "nproc": 5,
        "nofile": 16
    }
}

limits = DEFAULT_LIMITS[MODES[mode_index]].copy()

# ---------- Logging ----------
def log(msg, level="INFO"):
    ts = time.strftime("%H:%M:%S")
    line = f"{ts} [{level}] {msg}"
    log_lines.append(line)
    try:
        with open(LOG_FILE, "a") as f:
            f.write(line + "\n")
    except Exception:
        pass

# ---------- Helpers ----------
def human_uptime():
    if not start_time:
        return "00:00"
    s = int(time.time() - start_time)
    return f"{s//60:02}:{s%60:02}"

def wsl_to_win_path(wsl_path):
    try:
        out = subprocess.check_output(["wslpath", "-w", wsl_path])
        return out.decode().strip()
    except Exception:
        return None

def start_sandbox_from_ui():
    global sandbox_pid, sandbox_proc, status, start_time

    log("[ACTION] Launching sandbox terminal...")

    mode = MODES[mode_index].lower()  # open, restricted, locked
    win_proj = wsl_to_win_path(PROJECT_DIR)

    # Command the TUI will launch
    sandbox_cmd = ["wsl", "bash", SANDBOX_RUN, "sandbox_env", mode, "/bin/sh", "-i"]

    if win_proj:
        wt_cmd = ["wt.exe", "-w", "0", "nt", "-d", win_proj] + sandbox_cmd
    else:
        wt_cmd = ["wt.exe", "-w", "0", "nt", "-d", PROJECT_DIR] + sandbox_cmd

    try:
        subprocess.Popen(wt_cmd)
    except Exception as e:
        log(f"[ERR] Could not open Windows Terminal: {e}")

    time.sleep(1.0)
    start_time = time.time()
    status = "ACTIVE"
    log(f"[INFO] Sandbox launch requested with mode={mode}")


# ---------- Sandbox process detection (system-wide heuristic) ----------
def is_sandbox_related_process(p):
    """
    Return True if the process appears to belong to the sandbox environment.
    Heuristics:
      - command line contains 'sandbox_env'
      - cwd contains 'sandbox_env'
      - command line references the project dir
    """
    try:
        info_cmdline = p.cmdline() or []
        cmd = " ".join(info_cmdline)
    except Exception:
        cmd = ""
    try:
        cwd = p.cwd() or ""
    except Exception:
        cwd = ""

    # normalize
    cmd_low = cmd.lower()
    cwd_low = cwd.lower()

    # check for sandbox markers
    if "sandbox_env" in cmd_low or "sandbox_env" in cwd_low:
        return True
    # also check for project dir presence (a bit broader)
    if PROJECT_DIR and os.path.basename(PROJECT_DIR).lower() in cmd_low:
        return True
    return False

def get_all_sandbox_processes():
    """Return list of psutil.Process objects that appear to belong to the sandbox."""
    procs = []
    for p in psutil.process_iter(['pid','name']):
        try:
            if is_sandbox_related_process(p):
                procs.append(p)
        except Exception:
            continue
    return procs

# ---------- Limits & detection ----------
def check_limits_and_log(processes):
    """
    processes: list of psutil.Process objects
    - Logs if limits (UI-only) are exceeded per-process or in aggregate.
    """
    total_procs = 0
    total_fds = 0
    for p in processes:
        try:
            total_procs += 1
            try:
                fds = p.num_fds()
            except Exception:
                fds = 0
            total_fds += fds

            # cumulative cpu time
            times = p.cpu_times()

            cpu_secs = (times.user + times.system)
            if p.pid not in process_cpu_start:
                process_cpu_start[p.pid] = cpu_secs
            delta = cpu_secs - process_cpu_start.get(p.pid, cpu_secs)


            # rss MB
            try:
                rss_mb = p.memory_info().rss / (1024*1024)
            except Exception:
                rss_mb = 0.0

            if limits.get("cpu_time") is not None and delta > limits["cpu_time"]:
                log(f"[WARN] PID {p.pid} ({p.name()}) exceeded CPU time {delta:.1f}s > {limits['cpu_time']}s")
            if limits.get("ram_mb") is not None and rss_mb > limits["ram_mb"]:
                log(f"[WARN] PID {p.pid} ({p.name()}) using {rss_mb:.1f}MB > {limits['ram_mb']}MB")
        except psutil.NoSuchProcess:
            continue
        except Exception:
            continue

    if limits.get("nproc") is not None and total_procs > limits["nproc"]:
        log(f"[WARN] Sandbox processes {total_procs} > limit {limits['nproc']}")
    if limits.get("nofile") is not None and total_fds > limits["nofile"]:
        log(f"[WARN] Total open files {total_fds} > limit {limits['nofile']}")

# ---------- Draw helpers ----------
def draw_box(stdscr, y, x, h, w, title):
    try:
        stdscr.addstr(y, x, "┌" + "─"*(w-2) + "┐")
        for i in range(1, h-1):
            stdscr.addstr(y+i, x, "│" + " "*(w-2) + "│")
        stdscr.addstr(y+h-1, x, "└" + "─"*(w-2) + "┘")
        stdscr.addstr(y, x+2, f"{title}", curses.color_pair(1) | curses.A_BOLD)
    except Exception:
        pass

# ---------- UI input prompt ----------
def prompt_input(stdscr, y, x, prompt, initial=""):
    curses.echo()
    curses.curs_set(1)
    stdscr.attron(curses.A_REVERSE)
    stdscr.addstr(y, x, " " * 60)
    stdscr.addstr(y, x, prompt + initial)
    stdscr.refresh()
    win = curses.newwin(1, 60, y, x + len(prompt))
    try:
        val = win.getstr().decode().strip()
    except Exception:
        val = ""
    curses.noecho()
    curses.curs_set(0)
    stdscr.attroff(curses.A_REVERSE)
    return val

# ---------- Main UI ----------
def main(stdscr):
    global scroll, status, sandbox_pid, sandbox_proc, mode_index, limits, start_time, prev_seen

    # clear log on startup
    try:
        open(LOG_FILE, "w").close()
    except Exception:
        pass
    log_lines.clear()
    prev_seen = {}
    process_cpu_start.clear()
    log("[INFO] UI started")

    curses.curs_set(0)
    curses.start_color()
    curses.use_default_colors()
    # color pairs
    curses.init_pair(1, curses.COLOR_CYAN, -1)
    curses.init_pair(2, curses.COLOR_GREEN, -1)
    curses.init_pair(3, curses.COLOR_MAGENTA, -1)
    curses.init_pair(4, curses.COLOR_MAGENTA, -1)
    curses.init_pair(5, curses.COLOR_RED, -1)
    curses.init_pair(6, curses.COLOR_YELLOW, -1)
    curses.init_pair(7, curses.COLOR_GREEN, -1)
    curses.init_pair(8, curses.COLOR_MAGENTA, -1)
    curses.init_pair(9, curses.COLOR_RED, -1)
    curses.init_pair(10, curses.COLOR_BLUE, -1)
    curses.init_pair(11, curses.COLOR_MAGENTA, -1)

    stdscr.nodelay(True)
    stdscr.timeout(300)

    while True:
        stdscr.erase()
        h, w = stdscr.getmaxyx()

        # small bounds check
        if h < 28 or w < 100:
            stdscr.addstr(1, 2, "Increase terminal to at least 100x28", curses.color_pair(5))
            stdscr.refresh()
            time.sleep(0.3)
            continue

        # Control box
        draw_box(stdscr, 1, 2, 3, w-4, " Sandbox Control ")
        stdscr.addstr(2, 4, "[R] Run Sandbox", curses.color_pair(4))
        stdscr.addstr(2, 24, "[M] Mode", curses.color_pair(1))
        stdscr.addstr(2, 36, "[E] Edit Limits", curses.color_pair(1))
        stdscr.addstr(2, 56, "[Q] Quit", curses.color_pair(5))

        # Status box
        draw_box(stdscr, 5, 2, 6, w-4, " Status ")
        stdscr.addstr(6, 4, "State:", curses.color_pair(6))
        stdscr.addstr(6, 12, f"{status}", curses.color_pair(2) if status == "ACTIVE" else curses.color_pair(5))
        stdscr.addstr(6, 30, f"PID: {sandbox_pid if sandbox_pid else '-'}", curses.color_pair(6))
        stdscr.addstr(7, 4, f"Uptime: {human_uptime()}", curses.color_pair(6))
        stdscr.addstr(7, 30, f"Mode: {MODES[mode_index]}", curses.color_pair(1))

        # Limits box
        lb_y = 12
        lb_h = 7
        draw_box(stdscr, lb_y, 2, lb_h, w-4, " Resource Limits ")
        stdscr.addstr(lb_y+1, 4, f"CPU Time (s): ", curses.color_pair(6))
        stdscr.addstr(lb_y+1, 20, f"{str(limits['cpu_time']) if limits['cpu_time'] else 'unlimited'}", curses.color_pair(6))
        stdscr.addstr(lb_y+2, 4, f"RAM (MB): ", curses.color_pair(6))
        stdscr.addstr(lb_y+2, 20, f"{str(limits['ram_mb']) if limits['ram_mb'] else 'unlimited'}", curses.color_pair(6))
        stdscr.addstr(lb_y+3, 4, f"Max Processes: ", curses.color_pair(6))
        stdscr.addstr(lb_y+3, 20, f"{str(limits['nproc']) if limits['nproc'] else 'unlimited'}", curses.color_pair(6))
        stdscr.addstr(lb_y+4, 4, f"Max Open Files: ", curses.color_pair(6))
        stdscr.addstr(lb_y+4, 20, f"{str(limits['nofile']) if limits['nofile'] else 'unlimited'}", curses.color_pair(6))
        stdscr.addstr(lb_y+5, 4, "(Press M to change mode; E to edit limits)", curses.color_pair(3))

        # Processes box
        pbox_y = lb_y + lb_h + 1
        pbox_h = 11
        draw_box(stdscr, pbox_y, 2, pbox_h, w-4, " Sandbox Processes ")

        # collect sandbox-related processes (system-wide heuristic)
        raw_procs = get_all_sandbox_processes()

        # detect new/ended processes by pid set diff
        current_pids = {}
        for p in raw_procs:
            try:
                cmd = " ".join(p.cmdline()) if p.cmdline() else (p.name() or "<unknown>")
            except Exception:
                cmd = p.name() or "<unknown>"
            current_pids[p.pid] = cmd

        # log newly created processes
        for pid, cmd in current_pids.items():
            if pid not in prev_seen:
                prev_seen[pid] = cmd
                # seed cpu start (avoid immediate CPUsec violation)
                try:
                    times = psutil.Process(pid).cpu_times()
                    process_cpu_start[pid] = (times.user + times.system)
                except Exception:
                    process_cpu_start[pid] = 0.0
                log(f"[INFO] New process detected: PID {pid} - {cmd}")

        # log ended processes
        ended = [pid for pid in list(prev_seen.keys()) if pid not in current_pids]
        for pid in ended:
            log(f"[INFO] Process ended: PID {pid} - {prev_seen.get(pid, '')}")
            prev_seen.pop(pid, None)
            process_cpu_start.pop(pid, None)

        # build rows from current procs
        rows = []
        for p in raw_procs:
            try:
                cpu_percent = p.cpu_percent(interval=0.01)
                times = p.cpu_times()
                cpu_sec = round(times.user + times.system, 2)
                try:
                    mem_pct = p.memory_percent()
                except Exception:
                    mem_pct = 0.0
                try:
                    cmdline = p.cmdline()
                except Exception:
                    cmdline = []
                cmd = " ".join(cmdline) if cmdline else (p.name() or "<unknown>")
                state = p.status() if p.is_running() else "stopped"
                rows.append((p.pid, cpu_percent, cpu_sec, mem_pct, cmd, state, p))
            except psutil.NoSuchProcess:
                continue
            except Exception:
                continue

        # sort by CPU%
        rows.sort(key=lambda t: t[1], reverse=True)

        # Top summary line
        header_line = pbox_y + 1
        if rows:
            tpid, tcpu, tsec, tmem, tcmd, tstate, _ = rows[0]
            top_text = f"Top: {tcmd[:36]}   CPU:{tcpu:.1f}%   MEM:{tmem:.1f}%"
            stdscr.addstr(header_line, 4, top_text, curses.color_pair(5) | curses.A_BOLD)
            sandbox_pid = rows[0][0]  # show top pid in status for convenience
        else:
            stdscr.addstr(header_line, 4, "Top: -", curses.color_pair(3))

        # Column headers
        hdr_line = pbox_y + 2
        stdscr.addstr(hdr_line, 4,  "PID",     curses.color_pair(1))
        stdscr.addstr(hdr_line, 12, "CPU%",    curses.color_pair(1))
        stdscr.addstr(hdr_line, 20, "MEM%",    curses.color_pair(1))
        stdscr.addstr(hdr_line, 28, "CPUsec",  curses.color_pair(1))
        stdscr.addstr(hdr_line, 40, "COMMAND", curses.color_pair(1))
        stdscr.addstr(hdr_line, 90, "STATE",   curses.color_pair(1))

        # rows start below headers
        row = pbox_y + 3
        max_rows = pbox_h - 5

        if rows:
            for i, (pid, cpu_p, cpu_s, mem_p, cmd, state, pobj) in enumerate(rows[:max_rows]):
                y = row + i

                pid_col = curses.color_pair(6)
                cpu_col = curses.color_pair(9) if cpu_p >= 60 else (curses.color_pair(8) if cpu_p >= 20 else curses.color_pair(7))
                mem_col = curses.color_pair(9) if mem_p >= 20 else (curses.color_pair(8) if mem_p >= 5 else curses.color_pair(7))
                state_col = curses.color_pair(10) if state == "sleeping" else (curses.color_pair(11) if state == "zombie" else curses.color_pair(6))

                # safe command
                safe_cmd = (cmd[:36] + "...") if len(cmd) > 39 else cmd

                stdscr.addstr(y, 4,  f"{pid:<6}", pid_col)
                stdscr.addstr(y, 12, f"{cpu_p:>5.1f}", cpu_col)
                stdscr.addstr(y, 20, f"{mem_p:>5.1f}", mem_col)
                stdscr.addstr(y, 28, f"{cpu_s:>7.2f}", curses.color_pair(6))
                stdscr.addstr(y, 40, f"{safe_cmd:<{w-56}}", curses.color_pair(6))
                stdscr.addstr(y, 90, f"{state:<10}", state_col)
        else:
            stdscr.addstr(row, 4, "(no sandbox child processes detected)", curses.color_pair(3))

        # Check limits and log (UI-only)
        if rows:
            check_limits_and_log([r[6] for r in rows])

        # Logs area
        logs_top = pbox_y + pbox_h + 1
        logs_h = h - logs_top - 2
        draw_box(stdscr, logs_top, 2, logs_h, w-4, " Activity Logs ")
        visible = logs_h - 2
        total = len(log_lines)
        start = max(0, total - visible - scroll)
        end = max(0, total - scroll)
        y = logs_top + 1
        for ln in list(log_lines)[start:end]:
            color = curses.color_pair(2) if "[INFO]" in ln else (curses.color_pair(3) if "[WARN]" in ln else (curses.color_pair(5) if "[ERR]" in ln else curses.color_pair(6)))
            try:
                stdscr.addstr(y, 4, ln[:w-8], color)
            except Exception:
                pass
            y += 1

        stdscr.addstr(h-1, 2, "Tip: R=Run  M=Mode  E=Edit limits  Q=Quit  ↑/↓ Scroll", curses.color_pair(1))

        stdscr.refresh()

        # key handling
        key = stdscr.getch()
        if key in (ord('q'), ord('Q')):
            break
        if key in (ord('r'), ord('R')):
            start_sandbox_from_ui()
        if key in (ord('m'), ord('M')):
            mode_index = (mode_index + 1) % len(MODES)
            limits = DEFAULT_LIMITS[MODES[mode_index]].copy()
            log(f"[INFO] Mode switched to {MODES[mode_index]}; limits updated")
        if key in (ord('e'), ord('E')):
            # Blocking popup to edit limits
            stdscr.addstr(2, 2, " " * 60, curses.color_pair(1) | curses.A_REVERSE)
            stdscr.refresh()
            val = prompt_input(stdscr, 3, 4, "CPU Time (s; empty=unlimited): ", "" )
            try:
                limits['cpu_time'] = int(val) if val != "" else None
            except:
                limits['cpu_time'] = None
            val = prompt_input(stdscr, 4, 4, "RAM (MB; empty=unlimited): ", "" )
            try:
                limits['ram_mb'] = int(val) if val != "" else None
            except:
                limits['ram_mb'] = None
            val = prompt_input(stdscr, 5, 4, "Max processes (empty=unlimited): ", "" )
            try:
                limits['nproc'] = int(val) if val != "" else None
            except:
                limits['nproc'] = None
            val = prompt_input(stdscr, 6, 4, "Max open files (empty=unlimited): ", "" )
            try:
                limits['nofile'] = int(val) if val != "" else None
            except:
                limits['nofile'] = None
            log(f"[INFO] Limits edited via UI: CPU={limits['cpu_time']} RAM={limits['ram_mb']} NPROC={limits['nproc']} NOFILE={limits['nofile']}")

        if key == curses.KEY_UP:
            scroll = min(scroll + 1, max(0, total - visible))
        if key == curses.KEY_DOWN:
            scroll = max(scroll - 1, 0)

        time.sleep(0.03)


if __name__ == "__main__":
    curses.wrapper(main)
