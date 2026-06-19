# Headless self-test driver for the doki_theme plugin.
#
# Usage (Windows, from repo root):
#   & "<IDADIR>\idat.exe" -A "-Stools\selftest_driver.py" "<some.i64>"
#
# Invokes the plugin with arg == 1, which runs the in-process self-test
# (color conversion, palette completeness, catalog invariants, CSS
# generation). Exits 0 on PASS, non-zero on FAIL.
import ida_loader, ida_pro

# arg == 1 is the in-process self-test.
ok = ida_loader.load_and_run_plugin("doki_theme", 1)
ida_pro.qexit(0 if ok else 1)
