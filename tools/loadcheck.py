# Headless load check for the doki_theme plugin.
#
# Usage (Windows, from repo root):
#   & "<IDADIR>\idat.exe" -A "-Stools\loadcheck.py" "-Lbuild\loadcheck.log" <some.idb>
#
# Confirms init()/run()/unload print the expected "[doki]" lines without a GUI.
import ida_loader, ida_pro
print("=== doki load check: invoking plugin ===")
ida_loader.load_and_run_plugin("doki_theme", 0)  # init() runs here (PLUGIN_MULTI)
print("=== doki load check: done ===")
ida_pro.qexit(0)
