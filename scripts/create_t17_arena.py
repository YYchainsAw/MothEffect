"""Create only owned T17 graybox packages; verify actual saved geometry/nav/director."""
from pathlib import Path
import json
import math
import time
import unreal

root = Path(unreal.Paths.project_dir())
layout = json.loads((root / 'SourceArt/Levels/T17_Arena_Layout.json').read_text(encoding='utf-8'))
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
cache = root / 'Saved/Automation/T17'
cache.mkdir(parents=True, exist_ok=True)
marker = cache / 'CreatedPackages.json'
owned = json.loads(marker.read_text(encoding='utf-8')) if marker.exists() else []


def claim(path):
    if assets.does_asset_exist(path) and path not in owned:
        raise RuntimeError('Refuse to replace a non-T17-owned package: ' + path)
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


base = create('/Game/MothEffect/Maps/Materials/M_ArenaGraybox', unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(base)
color = unreal.MaterialEditingLibrary.create_material_expression(base, unreal.MaterialExpressionVectorParameter)
color.set_editor_property('parameter_name', 'Tint')
color.set_editor_property('default_value', unreal.LinearColor(.3, .3, .3, 1))
roughness = unreal.MaterialEditingLibrary.create_material_expression(base, unreal.MaterialExpressionConstant)
roughness.set_editor_property('r', .9)
unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
unreal.MaterialEditingLibrary.recompile_material(base)
save(base)
materials = {}
for name, rgb in [(z['name'], z['color']) for z in layout['zones']] + [('LowCover', [.39,.43,.45]), ('HighCover', [.29,.33,.36]), ('Platform', [.32,.34,.31])]:
    obj = create('/Game/MothEffect/Maps/Materials/MI_Arena' + name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(obj, base)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(obj, 'Tint', unreal.LinearColor(*rgb, 1))
    save(obj)
    materials[name] = obj

map_path = layout['map']
claim(map_path)
if assets.does_asset_exist(map_path):
    if not levels.load_level(map_path):
        raise RuntimeError('Cannot reload owned T17 map')
    for existing in actors.get_all_level_actors():
        if existing.get_actor_label().startswith('T17_') or isinstance(existing, (unreal.PlayerStart, unreal.NavMeshBoundsVolume, unreal.DirectionalLight, unreal.SkyAtmosphere, unreal.SkyLight)):
            if not actors.destroy_actor(existing):
                raise RuntimeError('Cannot rebuild owned actor ' + existing.get_actor_label())
elif not levels.new_level(map_path):
    raise RuntimeError('Cannot create owned T17 map')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
cube = unreal.load_asset('/Engine/BasicShapes/Cube')


def box(name, center, extent, material, pitch=0, support=True):
    obj = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center), unreal.Rotator(pitch=pitch, yaw=0, roll=0))
    obj.set_actor_label('T17_' + name)
    obj.static_mesh_component.set_static_mesh(cube)
    obj.static_mesh_component.set_collision_profile_name('BlockAll')
    obj.static_mesh_component.set_material(0, materials[material])
    obj.set_actor_scale3d(unreal.Vector(*(v / 50 for v in extent)))
    obj.set_editor_property('tags', ['T17Arena', 'MothColumnSurface'] if support else ['T17Arena'])
    return obj


for z in layout['zones']:
    box('Floor_' + z['name'], [z['center_x'],0,-10], [1000,2500,10], z['name'])
for kind, material in [('low', 'LowCover'), ('high', 'HighCover')]:
    extent = layout[kind + '_cover_extent']
    for index, (x,y) in enumerate(layout[kind + '_cover']):
        box(f'{kind.title()}Cover_{index+1}', [x,y,extent[2]], extent, material)
for name, center, extent in [
    ('WestWall', [-3025,0,150], [25,2550,150]),
    ('EastWall', [3025,0,150], [25,2550,150]),
    ('SouthWall', [0,-2525,150], [3000,25,150]),
    ('NorthWall', [0,2525,150], [3000,25,150])]:
    box(name, center, extent, 'HighCover', support=False)
p = layout['platform']
px, py = p['center']
ex, ey = p['extent']
height, run, thickness = p['height'], p['ramp_run'], p['ramp_thickness']
box('Platform', [px,py,height/2], [ex,ey,height/2], 'Platform')
angle = math.atan2(height, run)
for side, sign in [('West',1), ('East',-1)]:
    # Offset slab center by its normal so its TOP face meets both floor and deck exactly.
    top_x = px - sign * (ex + run/2)
    normal_x = -sign * math.sin(angle)
    center = [top_x - normal_x*thickness/2, py, height/2 - math.cos(angle)*thickness/2]
    box('Ramp_' + side, center, [math.hypot(run,height)/2,p['ramp_width']/2,thickness/2], 'Platform', math.degrees(angle)*sign)

director_class = unreal.load_class(None, '/Game/MothEffect/Framework/BP_WaveDirector.BP_WaveDirector_C')
if not director_class:
    raise RuntimeError('T16 saved director must exist')
director = actors.spawn_actor_from_class(director_class, unreal.Vector())
director.set_actor_label('T17_WaveDirector')
director.set_editor_property('auto_start', False)
actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(*layout['player_start']), unreal.Rotator(pitch=0, yaw=90, roll=0))
nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0,0,250))
nav.set_actor_scale3d(unreal.Vector(31,26,6))
settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)[0]
settings.set_editor_property('default_game_mode', unreal.load_class(None, '/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C'))
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1200), unreal.Rotator(pitch=-50, yaw=-30, roll=0))
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun.light_component.set_editor_property('atmosphere_sun_light', True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,1000))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)


def text(label, message, position, size=60, floor=True):
    sign = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*position), unreal.Rotator(pitch=90 if floor else 0,yaw=0 if floor else 90,roll=0))
    sign.set_actor_label('T17_' + label)
    sign.text_render.set_text(message)
    sign.text_render.set_world_size(size)
    sign.text_render.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


for i, (x,y) in enumerate(layout['spawn_points']):
    text(f'S{i+1}', f'S{i+1}', [x,y,8],80)
for z in layout['zones']:
    text(z['name'], z['name'].replace('_',' / '), [z['center_x']-350,0,5],100)
text('Controls', 'MOTH EFFECT / ARENA 01\nWASD Move | LMB Fire / Throw | E Pick up / Drop | R Reload\nConsole: MothStartEncounter / MothFlowStats / MothRestartEncounter', [-2500,-2400,200],32,False)
if not levels.save_current_level() or not levels.load_level(map_path):
    raise RuntimeError('Cannot save and reload T17 map')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
last_try = 0
reloaded = False
handle = None


def finish(error=None, result=None):
    if error:
        unreal.log_error('T17_ARENA_FAILED: ' + str(error))
    else:
        (cache / 'SavedArena.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        unreal.log('T17_ARENA_VERIFIED ' + json.dumps(result))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)


def verify(delta):
    global last_try, world, reloaded
    try:
        elapsed = time.monotonic() - started
        if elapsed < 2 or elapsed - last_try < 1:
            return
        last_try = elapsed
        if elapsed > 60:
            raise RuntimeError('Saved navigation/ramps incomplete after 60 seconds')
        if not reloaded:
            unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
        routes = []
        pairs = [{'name':f'S{i+1}_to_player','from':[x,y,90],'to':layout['player_start']} for i,(x,y) in enumerate(layout['spawn_points'])] + layout['navigation_routes']
        for pair in pairs:
            path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world,unreal.Vector(*pair['from']),unreal.Vector(*pair['to']))
            if not path or not path.is_valid() or path.is_partial():
                unreal.log(f'T17_NAV_WAIT {pair["name"]}')
                return
            pts = path.path_points
            if pair['name'].endswith('_ramp'):
                # A complete projected path is insufficient: it must end ON the raised deck
                # and pass through the requested approach, rather than taking the other side.
                via = pair['via_x']
                crosses_approach = any(
                    min(a.x,b.x) <= via <= max(a.x,b.x) and abs(b.x-a.x) > .01
                    and abs(a.y+(b.y-a.y)*(via-a.x)/(b.x-a.x)-py) < 140
                    for a,b in zip(pts,pts[1:]))
                if pts[-1].z < 110 or not crosses_approach:
                    unreal.log(f'T17_RAMP_WAIT {pair["name"]} {[str(v) for v in pts]}')
                    return
            if pair['name'] == 'high_cover_detour' and len(pts) < 3:
                raise RuntimeError('Saved route crosses solid high cover')
            routes.append({'name':pair['name'],'points':[[round(v.x,1),round(v.y,1),round(v.z,1)] for v in pts]})
        ds = unreal.GameplayStatics.get_all_actors_of_class(world,unreal.MothWaveDirector)
        expected = [f'/Game/MothEffect/Enemies/BP_Enemy{k}.BP_Enemy{k}_C' for k in ('Launcher','Bomb','Emitter')]
        actual = [c.get_path_name() if c else None for c in ds[0].get_editor_property('enemy_classes')] if len(ds)==1 else []
        if actual != expected or ds[0].get_editor_property('auto_start') or not ds[0].get_editor_property('spawn_warning_material'):
            raise RuntimeError(f'Saved director references invalid: {actual}')
        if not reloaded:
            if not levels.save_current_level() or not levels.load_level(map_path):
                raise RuntimeError('Cannot persist T17 static navigation')
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
            reloaded = True
            return
        finish(result={'map':map_path,'new_assets':len(owned),'static_nav_reloaded':True,'routes':routes,'enemy_classes':actual,'graphical_pie':False})
    except Exception as error:
        finish(error=error)


handle = unreal.register_slate_post_tick_callback(verify)
