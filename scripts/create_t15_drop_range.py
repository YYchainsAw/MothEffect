"""UE ticking editor script: own only new T15 packages; save a real drop range."""
from pathlib import Path
import json
import sys
import time
import unreal

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(unreal.Paths.project_dir()) / 'scripts'))
from t15_range_validation import validate_placed_enemies

assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
cache = Path(unreal.Paths.project_saved_dir()) / 'Automation/T15'
cache.mkdir(parents=True, exist_ok=True)
marker = cache / 'CreatedPackages.json'
owned = json.loads(marker.read_text(encoding='utf-8')) if marker.exists() else []

def claim(path):
    if assets.does_asset_exist(path) and path not in owned:
        raise RuntimeError('Refuse to replace existing non-T15-owned package: ' + path)
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

mat = create('/Game/MothEffect/Enemies/Materials/M_DropIndicator', unreal.Material, unreal.MaterialFactoryNew())
# Rebuild only this script's owned material when resuming a partial run.
unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
tint = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -250, 0)
tint.set_editor_property('parameter_name', 'Tint')
tint.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
unreal.MaterialEditingLibrary.connect_material_property(tint, '', unreal.MaterialProperty.MP_BASE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(tint, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(mat)
save(mat)

prism_path = '/Game/MothEffect/Enemies/Meshes/SM_DropLauncherPrism'
claim(prism_path)
if prism_path in owned:
    source = cache / 'DropLauncherPrism.obj'
    # Six original vertices, a triangular silhouette in Y/Z, extrusion along X.
    source.write_text('o DropLauncherPrism\n' + '\n'.join([
        'v -30 -50 -50', 'v -30 50 -50', 'v -30 0 50',
        'v 30 -50 -50', 'v 30 50 -50', 'v 30 0 50',
        'vt 0 0', 'vt 1 0', 'vt .5 1', 'vt 0 1',
        'vn -1 0 0', 'vn 1 0 0', 'vn 0 0 -1',
        'vn 0 .894427 .447214', 'vn 0 -.894427 .447214',
        'f 1/1/1 3/3/1 2/2/1', 'f 4/1/2 5/2/2 6/3/2',
        'f 1/1/3 2/2/3 5/3/3 4/4/3',
        'f 2/1/4 3/2/4 6/3/4 5/4/4',
        'f 3/1/5 1/2/5 4/3/5 6/4/5']) + '\n', encoding='utf-8')
    task = unreal.AssetImportTask()
    task.filename = str(source.resolve())
    task.destination_path = '/Game/MothEffect/Enemies/Meshes'
    task.destination_name = 'SM_DropLauncherPrism'
    task.automated = True
    task.save = True
    task.replace_existing = True  # claim() above restricts replacement to owned T15 assets.
    options = unreal.FbxImportUI()
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
    task.options = options
    asset_tools.import_asset_tasks([task])
prism = unreal.load_asset(prism_path)
if not prism:
    raise RuntimeError('Cannot import original triangular prism')

variants = [
    ('Launcher', unreal.DeviceKind.LAUNCHER, prism, unreal.Vector(.4, .4, .4), unreal.LinearColor(0, .8, 1, 1)),
    ('Bomb', unreal.DeviceKind.BOMB, unreal.load_asset('/Engine/BasicShapes/Sphere'), unreal.Vector(.4, .4, .4), unreal.LinearColor(1, .22, .01, 1)),
    ('Emitter', unreal.DeviceKind.EMITTER, unreal.load_asset('/Engine/BasicShapes/Cube'), unreal.Vector(.5, .24, .24), unreal.LinearColor(.5, .03, 1, 1)),
]
enemy_classes = []
for name, kind, shape, scale, color in variants:
    mi = create('/Game/MothEffect/Enemies/Materials/MI_Drop'+name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(mi, mat)
    # UE 5.8's setter updates the instance but always returns false. Verify readback.
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, 'Tint', color)
    actual = unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(mi, 'Tint')
    if any(abs(getattr(actual, c)-getattr(color, c)) > .001 for c in ('r', 'g', 'b', 'a')):
        raise RuntimeError('Material Tint readback mismatch: '+name)
    save(mi)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.load_class(None, f'/Game/MothEffect/Devices/BP_Device{name}.BP_Device{name}_C'))
    bp = create('/Game/MothEffect/Devices/Drops/BP_Drop'+name, unreal.Blueprint, factory)
    cls = unreal.load_class(None, f'/Game/MothEffect/Devices/Drops/BP_Drop{name}.BP_Drop{name}_C')
    cdo = unreal.get_default_object(cls)
    visual = cdo.get_editor_property('device_mesh')
    if visual.get_outer() != cdo:
        raise RuntimeError('Refuse to edit a shared parent component')
    cdo.modify(); visual.modify()
    visual.set_editor_property('static_mesh', shape)
    visual.set_editor_property('relative_scale3d', scale)
    visual.set_material(0, mi)
    if cdo.get_editor_property('device_kind') != kind:
        raise RuntimeError('Formal parent kind mismatch')
    save(bp)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.load_class(None, '/Game/MothEffect/Enemies/BP_EnemyBase.BP_EnemyBase_C'))
    bp = create('/Game/MothEffect/Enemies/BP_Enemy'+name, unreal.Blueprint, factory)
    enemy_cls = unreal.load_class(None, f'/Game/MothEffect/Enemies/BP_Enemy{name}.BP_Enemy{name}_C')
    cdo = unreal.get_default_object(enemy_cls)
    cdo.set_editor_property('drop_kind', kind)
    cdo.set_editor_property('drop_device_class', cls)
    save(bp)
    enemy_classes.append(enemy_cls)

map_path = '/Game/MothEffect/Tests/L_T15Drops'
claim(map_path)
if not assets.does_asset_exist(map_path):
    if not levels.new_level(map_path):
        raise RuntimeError('Cannot create owned T15 map')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    cube = unreal.load_asset('/Engine/BasicShapes/Cube')

    def box(label, location, extent, rotation=None):
        a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), rotation or unreal.Rotator())
        a.set_actor_label(label)
        a.static_mesh_component.set_static_mesh(cube)
        a.static_mesh_component.set_collision_profile_name('BlockAll')
        a.set_actor_scale3d(unreal.Vector(*(v/50 for v in extent)))
        a.set_editor_property('tags', ['MothColumnSurface'])
        return a

    box('T15_Floor', (0, 0, -10), (3500, 2400, 10))
    for x in (-1000, 1000):
        box('T15_BaySeparator', (x, 950, 200), (25, 1050, 200))
    box('T15_BombWall', (100, 700, 170), (20, 400, 170))
    box('T15_EmitterSlope20deg', (2000, 500, 100), (650, 450, 20), unreal.Rotator(20, 0, 0))
    for i, cls in enumerate(enemy_classes):
        location = unreal.Vector(-2000 + i*2000, 500 if i == 2 else 700, 213 if i == 2 else 90)
        e = actors.spawn_actor_from_class(cls, location, unreal.Rotator(0, -90, 0))
        e.set_actor_label('T15_Enemy_'+variants[i][0])
        sign = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(-2000 + i*2000, 1200, 240), unreal.Rotator(0, -90, 0))
        sign.text_render.set_world_size(32)
        sign.text_render.set_text('T15 D0'+str(i+1)+' / '+variants[i][0]+'\nDefeat: one fixed Dormant drop\nPickup and shoot/throw normally')
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-2000, -1000, 100), unreal.Rotator(0, 90, 0))
    nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 250))
    nav.set_actor_scale3d(unreal.Vector(35, 24, 6))
    settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)[0]
    settings.set_editor_property('default_game_mode', unreal.load_class(None, '/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C'))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1200), unreal.Rotator(-50, -30, 0))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('atmosphere_sun_light', True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    if not levels.save_current_level():
        raise RuntimeError('Cannot save T15 map')
if not levels.load_level(map_path):
    raise RuntimeError('Cannot reload saved T15 map for navigation initialization')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

# Validate actual saved actors, not only the Blueprint CDOs or newly spawned test actors.
# Repair the generator-owned instances even when resuming an existing map.
validate_placed_enemies(world, repair=True)

# Real editor ticks release the asynchronous load lock before the static navigation build.
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
last_try = 0
handle = None

def verify(delta):
    global last_try
    elapsed = time.monotonic() - started
    if elapsed < 2 or elapsed-last_try < 1:
        return
    last_try = elapsed
    if elapsed > 45:
        unreal.log_error('T15_RANGE_FAILED: navigation timeout')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        return
    unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
    path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, unreal.Vector(-2000, -700, 90), unreal.Vector(2000, -700, 90))
    if not path or not path.is_valid() or path.is_partial():
        return
    if not levels.save_current_level():
        raise RuntimeError('Cannot save built T15 navigation')
    if not levels.load_level(map_path):
        raise RuntimeError('Cannot reload saved T15 range for instance validation')
    saved_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    validate_placed_enemies(saved_world)
    unreal.log('T15_RANGE_VERIFIED '+json.dumps({'map': map_path, 'enemies':3, 'fixed_drop_pairs':3, 'new_assets':len(owned), 'navigation':True, 'points':[str(p) for p in path.path_points], 'scripted_deaths':False}))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)

handle = unreal.register_slate_post_tick_callback(verify)
