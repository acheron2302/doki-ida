# Phase 3 self-test driver: runs the plugin's color/palette self-tests.
#   & "<IDADIR>\idat.exe" -A "-S<abs>\tools\colortest.py" "-L<log>" <idb>
import ida_loader, ida_pro
print("=== doki color self-test ===")
ida_loader.load_and_run_plugin("doki_theme", 1)  # arg 1 -> run_selftest()
ida_pro.qexit(0)
