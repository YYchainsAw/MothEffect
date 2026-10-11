"""Run inside UE's Python commandlet to create a new, ordinary-actor T12 range.

No level script, automatic activation, target teleport or linked-device callback.
Refuses to replace a map that already exists.
"""
import json
import unreal

PATH = '/Game/MothEffect/Tests/L_T12Chains'
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
if assets.does_asset_exist(PATH):
    raise RuntimeError('Refuse to replace existing range: ' + PATH)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.new_level(PATH):
    raise RuntimeError('Unable to create range')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
material = unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial')
classes = {kind: unreal.load_class(None, '/Game/MothEffect/Devices/BP_Device' + kind + '.BP_Device' + kind + '_C')
           for kind in ['Emitter', 'Bomb', 'Launcher']}
if not all(classes.values()):
    raise RuntimeError('Missing delivered device Blueprints')


def box(label, position, half_size, ground=False):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position))
    actor.set_actor_label(label)
    mesh = actor.static_mesh_component
    mesh.set_static_mesh(cube)
    mesh.set_material(0, material)
    mesh.set_collision_profile_name('BlockAll')
    actor.set_actor_scale3d(unreal.Vector(*(n / 50.0 for n in half_size)))
    if ground:
        actor.set_editor_property('tags', ['MothColumnSurface'])
    return actor


def device(label, kind, position):
    actor = actors.spawn_actor_from_class(classes[kind], unreal.Vector(*position))
    actor.set_actor_label(label)
    actor.set_editor_property('log_device_events', True)
    return actor


def sign(label, text, position):
    actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(*position), unreal.Rotator(0, 180, 0))
    actor.set_actor_label(label)
    actor.text_render.set_text(text)
    actor.text_render.set_world_size(28.0)


box('WalkingFloor', (0, 3200, -130), (1900, 4300, 10), True)
stations = ['Direct', 'BombHit', 'BombMiss', 'ColumnHit', 'ColumnMiss']
for index, station in enumerate(stations):
    y = index * 1600.0
    box(station + '_Ground', (610, y, -10), (690, 700, 10), True)
    box(station + '_Backstop', (1100, y, 250), (10, 700, 250))
    for side in [-1, 1]:
        box(station + '_Boundary' + str(side), (500, y + side * 700, 100), (610, 10, 100))
    column = station.startswith('Column')
    device(station + '_START_D03', 'Emitter', (-190 if column else 0, y, 400 if column else 20))
    if station == 'Direct':
        device(station + '_TARGET_D02', 'Bomb', (250, y, 20))
        text = 'A: D03 -> D02\nShoot START from behind, nearly along +X'
    elif station.startswith('Bomb'):
        device(station + '_START_D02', 'Bomb', (250, y + 500, 20))
        device(station + '_TARGET_D03', 'Emitter', (250, y + (280 if station == 'BombHit' else 600), 20))
        text = 'B: blast moves D03 into fireline' if station == 'BombHit' else 'B control: same blast pushes D03 away'
        text += '\nStart D03 along +X, then shoot START D02 quickly'
    else:
        box(station + '_EmitterShelf', (-190, y, 370), (60, 60, 10), True)
        box(station + '_ShootingLedge', (-480, y - 100, 250), (110, 200, 10))
        for step in range(13):
            top = -120 + (step + 1) * 30
            box(station + '_Step' + str(step), (-1465 + step * 70, y - 200, (top - 120) / 2),
                (35, 90, (top + 120) / 2))
        # A starter at rest on the ground produces the same fixed base as an air starter.
        device(station + '_START_D01', 'Launcher', (250, y + 50, 20))
        device(station + '_TARGET_D03', 'Emitter', (250, y + (0 if station == 'ColumnHit' else 300), 20))
        text = 'C: column sends D03 through high fireline' if station == 'ColumnHit' else 'C control: target is outside column'
        text += '\nUse ledge: start high D03 along +X, then shoot START D01'
    sign(station + '_Instructions', text, (-80, y - 500, 160))

actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-450, -250, -20), unreal.Rotator(0, 0, 0))
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(-50, -30, 0))
sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('atmosphere_sun_light', True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)[0]
settings.set_editor_property('default_game_mode', unreal.load_class(None,
    '/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C'))
if not levels.save_current_level():
    raise RuntimeError('Unable to save range')
unreal.log('T12_RANGE_SAVED ' + json.dumps({'map': PATH, 'stations': stations,
    'device_count': len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DeviceBase)),
    'automatic_activation': False, 'ground_profile': 'BlockAll', 'tag': 'MothColumnSurface'}))
