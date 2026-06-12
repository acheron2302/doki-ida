# Phase 7/8 driver: apply a theme live (installs, activates, registers the
# nav-band colorizer). Plugin teardown at qexit must restore the colorizer.
import ida_loader, ida_pro
print("=== doki apply live ===")
ida_loader.load_and_run_plugin("doki_theme", 3)  # arg 3 -> ThemeApplier.apply
print("=== applied; exiting (teardown restores colorizer) ===")
ida_pro.qexit(0)
