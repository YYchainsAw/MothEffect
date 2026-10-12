"""UE editor script: own only new T16 packages and save a navigable flow range."""
from pathlib import Path
import json
import time
import unreal

assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
cache = Path(unreal.Paths.project_saved_dir()) / 'Automation/T16'
cache.mkdir(parents=True, exist_ok=True)
marker = cache / 'CreatedPackages.json'
owned = json.loads(marker.read_text(encoding='utf-8')) if marker.exists() else []

def claim(path):
    if assets.does_asset_exist(path) and path not in owned:
        raise RuntimeError('Refuse to replace a non-T16-owned package: ' + path)
    if path not in owned:
        owned.append(path)
        marker.write_text(json.dumps(owned, indent=2), encoding='utf-8')

def create(path, cls, factory):
    claim(path)
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    directory, name = path.rsplit('/', 1)
    obj = asset_tools.create_asset(name, directory, cls, factory)
    if not obj:
        raise RuntimeError('Cannot create ' + path)
    return obj

def save(obj):
    if not assets.save_loaded_asset(obj):
        raise RuntimeError('Cannot save ' + obj.get_path_name())

mat = create('/Game/MothEffect/Framework/M_SpawnWarning', unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
color.set_editor_property('constant', unreal.LinearColor(1, .4, 0, 1))
opacity = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
opacity.set_editor_property('r', .18)
unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
unreal.MaterialEditingLibrary.recompile_material(mat)
save(mat)

factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.MothWaveDirector)
bp = create('/Game/MothEffect/Framework/BP_WaveDirector', unreal.Blueprint, factory)
cls = unreal.load_class(None, '/Game/MothEffect/Framework/BP_WaveDirector.BP_WaveDirector_C')
cdo = unreal.get_default_object(cls)
enemy_classes = [unreal.load_class(None, f'/Game/MothEffect/Enemies/BP_Enemy{k}.BP_Enemy{k}_C') for k in ('Launcher', 'Bomb', 'Emitter')]
if not all(enemy_classes):
    raise RuntimeError('Missing saved T15 fixed-drop classes')
bp.modify()
cdo.modify()
cdo.set_editor_property('enemy_classes', enemy_classes)
cdo.set_editor_property('spawn_warning_material', mat)
cdo.set_editor_property('auto_start', False)  # T18 tutorial calls StartEncounter; T16 uses the console entry.
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
save(bp)

map_path = '/Game/MothEffect/Tests/L_T16Flow'
claim(map_path)
points = [(-1800, 1800), (-800, 1800), (800, 1800), (-800, -1600), (1800, 1800), (2600, 200), (1800, -1800), (200, -1800)]
if not assets.does_asset_exist(map_path):
    if not levels.new_level(map_path):
        raise RuntimeError('Cannot create T16 map')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -10))
    floor.set_actor_label('T16_Floor_60x50m')
    floor.static_mesh_component.set_static_mesh(cube)
    floor.static_mesh_component.set_collision_profile_name('BlockAll')
    floor.set_actor_scale3d(unreal.Vector(60, 50, .2))
    floor.set_editor_property('tags', ['MothColumnSurface'])
    director = actors.spawn_actor_from_class(cls, unreal.Vector())
    director.set_actor_label('T16_WaveDirector')
    for i, (x, y) in enumerate(points):
        sign = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(x, y, 8), unreal.Rotator(90, 0, 0))
        sign.set_actor_label(f'T16_S{i+1}')
        sign.text_render.set_text(f'S{i+1}')
        sign.text_render.set_world_size(80)
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-2200, -1800, 100), unreal.Rotator(0, 90, 0))
    nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 250))
    nav.set_actor_scale3d(unreal.Vector(31, 26, 6))
    settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)[0]
    settings.set_editor_property('default_game_mode', unreal.load_class(None, '/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C'))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1200), unreal.Rotator(-50, -30, 0))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('atmosphere_sun_light', True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    sign = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(-2400, -2350, 220), unreal.Rotator(0, 90, 0))
    sign.text_render.set_world_size(32)
    sign.text_render.set_text('T16 FLOW RANGE\nConsole: MothStartEncounter / MothFlowStats / MothRestartEncounter\n4 -> 3+3 -> 4+4; tutorial/HUD and final arena follow T18/T17')
    if not levels.save_current_level():
        raise RuntimeError('Cannot save T16 map')
if not levels.load_level(map_path):
    raise RuntimeError('Cannot reload T16 map')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
# A resumed map may contain the pre-compilation defaults from an interrupted first run.
# Repair only this script's owned director instance, and verify after another reload below.
saved_directors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MothWaveDirector)
if len(saved_directors) != 1:
    raise RuntimeError('Owned T16 map must have exactly one director')
if not actors.destroy_actor(saved_directors[0]):
    raise RuntimeError('Cannot replace owned T16 director instance')
new_director = actors.spawn_actor_from_class(cls, unreal.Vector())
new_director.set_actor_label('T16_WaveDirector')
if not levels.save_current_level() or not levels.load_level(map_path):
    raise RuntimeError('Cannot save and reload owned T16 director references')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
last_try = 0
handle = None

def verify_impl(delta):
    global last_try
    elapsed = time.monotonic() - started
    if elapsed < 2 or elapsed - last_try < 1:
        return
    last_try = elapsed
    if elapsed > 60:
        unreal.log_error('T16_RANGE_FAILED: navigation timeout')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        return
    unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
    routes = []
    for x, y in points:
        path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, unreal.Vector(x, y, 92), unreal.Vector(-2200, -1800, 100))
        if not path or not path.is_valid() or path.is_partial():
            return
        routes.append(len(path.path_points))
    loaded = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MothWaveDirector)
    actual = [c.get_path_name() if c else None for c in loaded[0].get_editor_property('enemy_classes')] if len(loaded) == 1 else []
    expected = [c.get_path_name() for c in enemy_classes]
    if len(loaded) != 1 or actual != expected:
        current_cdo = unreal.get_default_object(unreal.load_class(None, '/Game/MothEffect/Framework/BP_WaveDirector.BP_WaveDirector_C'))
        defaults = [c.get_path_name() if c else None for c in current_cdo.get_editor_property('enemy_classes')]
        raise RuntimeError(f'Saved director mismatch: actors={len(loaded)} actual={actual} defaults={defaults} expected={expected}')
    if not levels.save_current_level():
        raise RuntimeError('Cannot save T16 static navigation')
    unreal.log('T16_RANGE_VERIFIED ' + json.dumps({'map': map_path, 'new_assets': len(owned), 'spawn_routes': routes, 'ready': True, 'graphical_pie': False}))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)

def verify(delta):
    try:
        verify_impl(delta)
    except Exception as error:
        unreal.log_error('T16_RANGE_FAILED: ' + str(error))
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)

handle = unreal.register_slate_post_tick_callback(verify)
