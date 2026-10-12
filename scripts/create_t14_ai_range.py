"""UE Python commandlet: create only new T14 assets; never replace existing packages."""
import json
import unreal

assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
BP = '/Game/MothEffect/Enemies/BP_EnemyBase'
MAP = '/Game/MothEffect/Tests/L_T14AI'
if assets.does_asset_exist(BP) or assets.does_asset_exist(MAP):
    raise RuntimeError('Refuse to replace existing T14 packages')
factory = unreal.BlueprintFactory()
factory.set_editor_property('parent_class', unreal.EnemyBase)
bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset('BP_EnemyBase', '/Game/MothEffect/Enemies', unreal.Blueprint, factory)
if not bp:
    raise RuntimeError('Cannot create enemy Blueprint')
cls = unreal.load_class(None, BP + '.BP_EnemyBase_C')
cdo = unreal.get_default_object(cls)
cdo.set_editor_property('projectile_class', unreal.load_class(None, '/Game/MothEffect/Combat/BP_RuleProjectile.BP_RuleProjectile_C'))
cdo.set_editor_property('log_enemy_events', True)
if not cdo.get_editor_property('projectile_class') or not assets.save_loaded_asset(bp):
    raise RuntimeError('Cannot configure/save formal P01 enemy Blueprint')
if not levels.new_level(MAP):
    raise RuntimeError('Cannot create T14 range')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
cube = unreal.load_asset('/Engine/BasicShapes/Cube')

def box(label, location, extent):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_collision_profile_name('BlockAll')
    a.set_actor_scale3d(unreal.Vector(*(v / 50 for v in extent)))
    a.set_editor_property('tags', ['MothColumnSurface'])
    return a

box('T14_Floor', (0, 0, -10), (2500, 1800, 10))
box('T14_Cover', (0, 0, 200), (30, 600, 200))
box('T14_LowCover', (-650, -1050, 45), (180, 30, 45))
box('T14_Platform', (-650, 1000, 60), (220, 180, 60))
for i in range(4):
    top = (i + 1) * 30
    box('T14_Step' + str(i), (-650, 550 + i * 70, top / 2), (220, 35, top / 2))
actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-1000, -750, 100), unreal.Rotator(0, 0, 0))
enemy = actors.spawn_actor_from_class(cls, unreal.Vector(1200, -750, 90), unreal.Rotator(0, 180, 0))
enemy.set_actor_label('T14_Enemy_Shooter')
nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 200))
nav.set_actor_label('T14_Static_NavBounds')
nav.set_actor_scale3d(unreal.Vector(25, 18, 5))
_, extent = nav.get_actor_bounds(False)
if extent.x < 2000 or extent.y < 1400:
    raise RuntimeError('Navigation volume brush missing or undersized: ' + str(extent))
settings = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)[0]
settings.set_editor_property('default_game_mode', unreal.load_class(None, '/Game/MothEffect/Framework/BP_MothGameMode.BP_MothGameMode_C'))
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(-50, -30, 0))
sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('atmosphere_sun_light', True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
sign = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(-900, -1300, 200), unreal.Rotator(0, 0, 0))
sign.set_actor_label('T14_Instructions')
sign.text_render.set_world_size(28)
sign.text_render.set_text('T14 AI / TC21\nObserve pursuit, FIRING warning and P01\nHide behind cover, emerge, jump/platform\nW01: three normal hits to defeat\nMothSpawnEnemy / MothResetEnemies / MothEnemyStats')
unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
if not levels.save_current_level():
    raise RuntimeError('Cannot save T14 range')
unreal.log('T14_RANGE_SAVED ' + json.dumps({'map': MAP, 'blueprint': BP, 'enemies': 1, 'static_navigation': True, 'scripted_shots': False, 'fixed_drops': 'T15', 'asset_source': 'native cube + existing P01/game mode'}, ensure_ascii=False))
