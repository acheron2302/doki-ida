# Phase 6 driver: install all bundled themes and activate one.
#   & "<IDADIR>\idat.exe" -A "-S<abs>\tools\install_themes.py" "-L<log>" <idb>
import ida_loader, ida_pro
print("=== doki install themes ===")
ida_loader.load_and_run_plugin("doki_theme", 2)  # arg 2 -> install + activate
ida_pro.qexit(0)
