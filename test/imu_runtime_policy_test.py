from pathlib import Path


project_root = Path(__file__).resolve().parents[1]
imu_source = (project_root / "src/sensors/sticky_imu.cpp").read_text()
app_source = (project_root / "src/app/prayer_app.cpp").read_text()
display_source = (project_root / "src/display/sticky_display.cpp").read_text()

assert "kMotionStartDeltaG = 0.18F" in imu_source
assert "kSettleSampleCount = 5" in imu_source
assert "kFourDimension70DegreeThreshold = 0xA0" in imu_source
assert "sticky_shake_detector_update" not in imu_source
assert "if (imu_state.moving)" not in app_source
assert "state=unchanged rotation=%u result=resleep" in app_source
assert "s_transition_cleanup_pending.store(true" in display_source
assert "s_transition_cleanup_pending.store(false" in display_source
