"""Run with -ExecutePythonScript in a ticking editor, after creating the T14 range."""
import json
import time
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/MothEffect/Tests/L_T14AI'):
    raise RuntimeError('Cannot reload T14 range')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
last_try = 0
handle = None

def finish(delta):
    global last_try
    elapsed = time.monotonic() - started
    if elapsed < 2 or elapsed - last_try < 1:
        return
    last_try = elapsed
    if elapsed > 45:
        unreal.log_error('T14_NAV_FAILED timeout waiting for valid built path')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        return
    # Real editor/navigation ticks release AsyncLoadLock; no manual lock override.
    unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
    path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, unreal.Vector(1000, 0, 90), unreal.Vector(-1000, 0, 90))
    if not path or not path.is_valid() or path.is_partial():
        return
    points = path.path_points
    if len(points) < 3 or not any(abs(p.y) > 600 for p in points):
        raise RuntimeError('Path did not route around saved static cover')
    if not levels.save_current_level():
        raise RuntimeError('Cannot save built navigation')
    unreal.log('T14_NAV_VERIFIED ' + json.dumps({'map': '/Game/MothEffect/Tests/L_T14AI', 'valid': True, 'partial': False, 'points': [str(p) for p in points], 'elapsed': elapsed, 'saved': True}))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)

handle = unreal.register_slate_post_tick_callback(finish)
