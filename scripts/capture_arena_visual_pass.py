"""Capture the same saved arena cameras before/after the first visual pass.

Prepare with stage_arena_visual_pass.py, then use two fresh rendered editors:
  MOTH_VISUAL_CAPTURE_MODE=Before (or After)
  UnrealEditor.exe <stage>/MothEffect.uproject -d3d12 -unattended -NoSound
      -ExecutePythonScript=<source>/scripts/capture_arena_visual_pass.py

The stage is required; no packages are saved. The after director is transient.
Screenshots establish static appearance only; temporal ghosting needs playtesting.
"""
from pathlib import Path
import gc
import hashlib
import json
import os
import time
import traceback
import unreal


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Saved/Automation/ArenaVisualPass"
STAGE = OUTPUT / "Stage_20261018_01"
MAP = "/Game/MothEffect/Maps/L_Arena_SoulCity"
MAP_FILE = "Content/MothEffect/Maps/L_Arena_SoulCity.umap"
MODE = os.environ.get("MOTH_VISUAL_CAPTURE_MODE", "").strip()
if MODE not in ("Before", "After"):
    raise RuntimeError("Set MOTH_VISUAL_CAPTURE_MODE to Before or After")
if Path(unreal.Paths.project_dir()).resolve() != STAGE.resolve():
    raise RuntimeError("Visual capture is restricted to its isolated arena stage")
COMMAND_LINE = unreal.SystemLibrary.get_command_line()
if "-d3d12" not in COMMAND_LINE.lower() or "-nullrhi" in COMMAND_LINE.lower():
    raise RuntimeError("This comparison requires a rendered D3D12 editor")


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def fingerprint():
    return {p.relative_to(ROOT).as_posix(): sha(p)
            for p in sorted((ROOT / "Content/MothEffect").rglob("*")) if p.is_file()}


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding="utf-8")


def require(condition, message):
    if not condition:
        raise RuntimeError(message)
    return condition


def vector(value):
    return [float(value.x), float(value.y), float(value.z)]


def rotation(value):
    return [float(value.pitch), float(value.yaw), float(value.roll)]


INT_CVARS = (
    "r.AntiAliasingMethod", "r.HighResScreenshotDelay",
    "r.DynamicRes.OperationMode", "r.Streaming.FullyLoadUsedTextures",
    "r.DefaultFeature.AutoExposure", "r.DefaultFeature.MotionBlur",
    "r.MotionBlurQuality", "r.Tonemapper.Quality", "r.Lumen.DiffuseIndirect.Allow",
    "r.Lumen.Reflections.Allow", "sg.ViewDistanceQuality", "sg.AntiAliasingQuality",
    "sg.ShadowQuality", "sg.GlobalIlluminationQuality", "sg.ReflectionQuality",
    "sg.PostProcessQuality", "sg.TextureQuality", "sg.EffectsQuality", "sg.FoliageQuality",
    "sg.ShadingQuality")
FLOAT_CVARS = ("r.ScreenPercentage", "r.DefaultFeature.AutoExposure.Bias",
               "r.TSR.History.ScreenPercentage", "r.Streaming.PoolSize")


def console_values():
    values = {name: unreal.SystemLibrary.get_console_variable_int_value(name) for name in INT_CVARS}
    values.update({name: unreal.SystemLibrary.get_console_variable_float_value(name) for name in FLOAT_CVARS})
    return values


def light_snapshot():
    result = []
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Light):
        component = actor.get_component_by_class(unreal.LightComponent)
        if component:
            color = component.get_editor_property("light_color")
            result.append(dict(label=actor.get_actor_label(), actor=actor.get_path_name(),
                               intensity=float(component.get_editor_property("intensity")),
                               color=[int(color.r), int(color.g), int(color.b), int(color.a)]))
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SkyLight):
        component = actor.get_component_by_class(unreal.SkyLightComponent)
        if component:
            result.append(dict(label=actor.get_actor_label(), actor=actor.get_path_name(),
                               intensity=float(component.get_editor_property("intensity"))))
    return result


OUTPUT.mkdir(parents=True, exist_ok=True)
baseline = json.loads((OUTPUT / "OwnedAssetsBefore.json").read_text(encoding="utf-8"))
before = fingerprint()
require(before == baseline, "Source assets differ from the prepared stage baseline")
started = time.monotonic()
started_utc = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
handle = None
task = None
issuing = False
finished = False
world = None
director = None
shots = []
camera_index = 0
camera_started = 0.0
camera_frames = 0
shader_clear_frames = 0
requested_at = None
cameras = []
lights_before = []
lights_after = []
console_before = {}
console_capture = {}
unreal.EditorPythonScripting.set_keep_python_script_alive(True)


def finish(error=None):
    global finished, handle, task, world, director, cameras
    if finished:
        return
    finished = True
    after = fingerprint()
    changed = sorted(p for p in before.keys() | after.keys() if before.get(p) != after.get(p))
    if changed:
        error = RuntimeError("Owned assets changed during capture: " + ", ".join(changed))
    result = dict(success=error is None, mode=MODE, error=str(error) if error else None,
                  stage=str(STAGE), map=MAP, renderer="D3D12 rendered editor; fresh process per mode",
                  started_utc=started_utc, elapsed_seconds=round(time.monotonic() - started, 2),
                  image_size=[1600, 900], screenshots=shots,
                  map_sha256_before=before.get(MAP_FILE), map_sha256_after=after.get(MAP_FILE),
                  owned_sha256_before=before, owned_sha256_after=after,
                  changed_owned_assets=changed, saved_packages=[],
                  console_variables_before=console_before, actual_console_variables=console_capture,
                  lights_before=lights_before, lights_after=lights_after,
                  look_actor=director.get_class().get_path_name() if director else None,
                  look_parameters={name: director.get_editor_property(name) for name in (
                      "sun_intensity", "sky_intensity", "fill_intensity", "exposure_compensation",
                      "saturation", "contrast", "shadow_gamma", "bloom_intensity", "post_process_priority"
                  )} if director else None,
                  camera_warmup_seconds=30, camera_minimum_frames=120,
                  shader_clear_minimum_frames=60, high_res_runup_frames=64,
                  note="Saved camera actors; no PIE. Static comparison does not measure temporal ghosting.")
    write(OUTPUT / (MODE + "_CaptureResult.json"), result)
    combined = {"map": MAP, "modes": {}}
    for name in ("Before", "After"):
        path = OUTPUT / (name + "_CaptureResult.json")
        if path.is_file():
            combined["modes"][name] = json.loads(path.read_text(encoding="utf-8"))
    combined["success"] = (len(combined["modes"]) == 2
                           and all(entry["success"] for entry in combined["modes"].values()))
    write(OUTPUT / "CaptureResult.json", combined)
    if handle is not None:
        unreal.unregister_slate_post_tick_callback(handle)
        handle = None
    if error:
        unreal.log_error("ARENA_VISUAL_CAPTURE_FAILED " + str(error))
    else:
        unreal.log("ARENA_VISUAL_CAPTURE_COMPLETE " + MODE + " " + str(len(shots)))
    # Release Python wrappers while Unreal is still alive, before interpreter finalization.
    task = None
    world = None
    director = None
    cameras = []
    for name in ("existing_cameras", "levels", "editor", "actors", "cls"):
        globals().pop(name, None)
    # Discard this editor-only lighting actor and its modified world without saving.
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    gc.collect()
    # The -ExecutePythonScript runner defers QUIT_EDITOR until this Slate callback returns.
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)


def select_camera(index):
    global camera_started, camera_frames, shader_clear_frames
    camera = cameras[index]
    # Pilot throughout warmup so streaming and exposure use the saved camera's FOV.
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).pilot_level_actor(camera)
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera.get_actor_location(), camera.get_actor_rotation())
    camera_started = time.monotonic()
    camera_frames = 0
    shader_clear_frames = 0
    unreal.log("ARENA_VISUAL_WARMUP " + MODE + " " + camera.get_actor_label())


def tick(delta):
    global task, issuing, requested_at, camera_index, camera_frames, shader_clear_frames
    if finished or issuing:
        return
    try:
        if time.monotonic() - started > 900:
            raise RuntimeError("Arena visual capture timed out after 15 minutes")
        if task:
            if not task.is_task_done():
                return
            camera = cameras[camera_index]
            name = camera.get_actor_label().removeprefix("SC_View_")
            image = OUTPUT / (MODE + "_" + name + ".png")
            require(image.is_file() and image.stat().st_size > 1000, "Screenshot produced no real image: " + str(image))
            require(image.stat().st_mtime >= requested_at - 1.0, "Screenshot file is older than this request")
            shots.append(dict(path=str(image), camera=camera.get_actor_label(),
                              position=vector(camera.get_actor_location()), rotation=rotation(camera.get_actor_rotation()),
                              field_of_view=float(camera.camera_component.get_editor_property("field_of_view")),
                              requested_unix_time=requested_at, warmed_seconds=round(time.monotonic()-camera_started, 2),
                              viewport_frames=camera_frames, shader_clear_frames=shader_clear_frames,
                              actual_console_variables=console_values()))
            task = None
            camera_index += 1
            if camera_index == len(cameras):
                finish()
                return
            select_camera(camera_index)
            return
        camera_frames += 1
        shader_check = getattr(unreal.AutomationLibrary, "are_shaders_compiling", None)
        if shader_check and shader_check():
            shader_clear_frames = 0
            return
        shader_clear_frames += 1
        if time.monotonic()-camera_started < 30 or camera_frames < 120 or shader_clear_frames < 60:
            return
        camera = cameras[camera_index]
        name = camera.get_actor_label().removeprefix("SC_View_")
        issuing = True
        try:
            requested_at = time.time()
            task = unreal.AutomationLibrary.take_high_res_screenshot(
                1600, 900, str(OUTPUT / (MODE + "_" + name + ".png")), camera=camera, delay=3.0)
        finally:
            # Taking a screenshot can pump Slate recursively before returning its task.
            issuing = False
        require(task and task.is_valid_task(), "Editor rejected the screenshot task")
        unreal.log("ARENA_VISUAL_CAPTURE_REQUEST " + MODE + " " + name)
    except Exception as exc:
        unreal.log_error(traceback.format_exc())
        finish(exc)


try:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    require(levels.load_level(MAP), "Cannot load the saved formal arena")
    world = require(editor.get_editor_world(), "No editor world")
    console_before = console_values()
    lights_before = light_snapshot()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors.clear_actor_selection_set()
    existing_cameras = {a.get_actor_label(): a for a in
                        unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CameraActor)}
    cameras = [require(existing_cameras.get("SC_View_" + name), "Missing saved camera " + name)
               for name in ("Entrance", "Platform")]
    if MODE == "After":
        cls = require(unreal.load_class(None, "/Script/MothEffect.MothArenaVisualDirector"),
                      "After capture requires newly compiled MothArenaVisualDirector")
        director = require(actors.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0)),
                           "Cannot spawn transient visual director")
        director.set_actor_label("ArenaVisualPass_Unsaved_AfterOnly")
        director.apply_look()
        require(director.was_look_applied(), "Visual director did not apply the after look")
        require(director.get_post_process_component(), "Visual director has no post-process component")
    for name in INT_CVARS:
        if name.startswith("sg."):
            unreal.SystemLibrary.execute_console_command(world, name + " 3")
    for command in ("r.ScreenPercentage 100", "r.DynamicRes.OperationMode 0",
                    "r.HighResScreenshotDelay 64", "r.Streaming.FullyLoadUsedTextures 1",
                    "DisableAllScreenMessages", "r.AntiAliasingMethod " + ("0" if MODE == "Before" else "4")):
        unreal.SystemLibrary.execute_console_command(world, command)
    console_capture = console_values()
    require(console_capture["r.AntiAliasingMethod"] == (0 if MODE == "Before" else 4), "AA CVar was not applied")
    require(console_capture["r.ScreenPercentage"] == 100.0, "Screen percentage is not 100")
    lights_after = light_snapshot()
    set_realtime = getattr(levels, "editor_set_viewport_realtime", None)
    if set_realtime:
        set_realtime(False)
        set_realtime(True)
    set_game_view = getattr(levels, "editor_set_game_view", None)
    if set_game_view:
        set_game_view(True)
    select_camera(0)
    handle = unreal.register_slate_post_tick_callback(tick)
except Exception as exc:
    unreal.log_error(traceback.format_exc())
    finish(exc)
