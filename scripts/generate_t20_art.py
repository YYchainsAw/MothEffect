"""Generate MothEffect's original, replaceable T20 low-poly mesh sources.

Only Python's standard library is used. Coordinates are Unreal centimeters:
Z up, +X forward. Running this script explicitly overwrites its own generated
OBJ/MTL/manifest/README files in SourceArt/PrototypeArt. Copy user edits elsewhere
before regenerating. --check validates existing sources without writing files.
No third-party meshes, textures, recordings, or code are embedded in these assets.
"""

from argparse import ArgumentParser
from dataclasses import dataclass
from hashlib import sha256
import json
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'SourceArt/PrototypeArt'
VERSION = '1.0'
SLOTS = ('Metal', 'Copper', 'Glow')
EPSILON = 1.e-8


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def mul(a, scalar):
    return tuple(x * scalar for x in a)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def length(a):
    return math.sqrt(dot(a, a))


def unit(a):
    magnitude = length(a)
    assert magnitude > EPSILON, 'Degenerate direction or triangle'
    return mul(a, 1 / magnitude)


def center(points):
    return tuple(sum(p[axis] for p in points) / len(points) for axis in range(3))


def along_axis(point, axis='Z'):
    """Rotate Z-axis construction into X/Y with handedness-preserving cycles."""
    x, y, z = point
    if axis == 'X':
        return z, x, y
    if axis == 'Y':
        return y, z, x
    assert axis == 'Z'
    return point


@dataclass(frozen=True)
class Triangle:
    points: tuple
    normal: tuple
    uv: tuple
    slot: str


class Mesh:
    def __init__(self, name, purpose, collision_policy):
        self.name = name
        self.purpose = purpose
        self.collision_policy = collision_policy
        self.triangles = []

    def face(self, points, slot, outward):
        """Add a convex flat polygon, orient it outward, and map its local UVs."""
        assert slot in SLOTS
        points = tuple(tuple(float(x) for x in p) for p in points)
        assert len(points) >= 3
        normal = unit(cross(sub(points[1], points[0]), sub(points[2], points[0])))
        assert abs(dot(normal, outward)) > EPSILON, 'Unknown face orientation'
        if dot(normal, outward) < 0:
            points = tuple(reversed(points))
            normal = mul(normal, -1)
        tangent = unit(sub(points[1], points[0]))
        bitangent = cross(normal, tangent)
        projected = [(dot(sub(p, points[0]), tangent),
                      dot(sub(p, points[0]), bitangent)) for p in points]
        lower = tuple(min(p[a] for p in projected) for a in range(2))
        upper = tuple(max(p[a] for p in projected) for a in range(2))
        assert all(upper[a] - lower[a] > EPSILON for a in range(2))
        uv = [tuple((p[a] - lower[a]) / (upper[a] - lower[a])
                    for a in range(2)) for p in projected]
        for i in range(1, len(points) - 1):
            indices = (0, i, i + 1)
            tri = tuple(points[j] for j in indices)
            assert dot(cross(sub(tri[1], tri[0]), sub(tri[2], tri[0])), normal) > EPSILON
            self.triangles.append(Triangle(tri, normal,
                                           tuple(uv[j] for j in indices), slot))

    def box(self, lower, upper, slot='Metal'):
        x0, y0, z0 = lower
        x1, y1, z1 = upper
        assert x1 > x0 and y1 > y0 and z1 > z0
        faces = (
            ([(x0,y0,z0), (x0,y1,z0), (x0,y1,z1), (x0,y0,z1)], (-1,0,0)),
            ([(x1,y0,z0), (x1,y1,z0), (x1,y1,z1), (x1,y0,z1)], (1,0,0)),
            ([(x0,y0,z0), (x1,y0,z0), (x1,y0,z1), (x0,y0,z1)], (0,-1,0)),
            ([(x0,y1,z0), (x1,y1,z0), (x1,y1,z1), (x0,y1,z1)], (0,1,0)),
            ([(x0,y0,z0), (x1,y0,z0), (x1,y1,z0), (x0,y1,z0)], (0,0,-1)),
            ([(x0,y0,z1), (x1,y0,z1), (x1,y1,z1), (x0,y1,z1)], (0,0,1)),
        )
        for points, normal in faces:
            self.face(points, slot, normal)

    def prism_xz(self, polygon, y0, y1, slot='Metal'):
        """Extrude a convex X/Z outline across Y; useful for arrows and shields."""
        centroid = (sum(p[0] for p in polygon) / len(polygon), (y0+y1)/2,
                    sum(p[1] for p in polygon) / len(polygon))
        back = [(x, y0, z) for x, z in polygon]
        front = [(x, y1, z) for x, z in polygon]
        self.face(back, slot, (0,-1,0))
        self.face(front, slot, (0,1,0))
        for i in range(len(polygon)):
            j = (i + 1) % len(polygon)
            points = [back[i], back[j], front[j], front[i]]
            self.face(points, slot, sub(center(points), centroid))

    def cylinder(self, radius, start, end, slot='Metal', segments=8,
                 axis='Z', offset=(0,0,0)):
        assert radius > 0 and end > start and segments >= 3
        def transform(point):
            return add(along_axis(point, axis), offset)
        rings = [[transform((radius * math.cos(i * 2*math.pi/segments),
                             radius * math.sin(i * 2*math.pi/segments), z))
                  for i in range(segments)] for z in (start, end)]
        self.face(rings[0], slot, along_axis((0,0,-1), axis))
        self.face(rings[1], slot, along_axis((0,0,1), axis))
        axial_center = transform((0,0,(start+end)/2))
        for i in range(segments):
            j = (i + 1) % segments
            points = [rings[0][i], rings[0][j], rings[1][j], rings[1][i]]
            self.face(points, slot, sub(center(points), axial_center))

    def torus(self, radius, tube, slot, axis='Z', offset=(0,0,0),
              segments=12, sides=4):
        assert radius > tube > 0 and segments >= 3 and sides >= 3
        local = [[((radius+tube*math.cos(j*2*math.pi/sides))*math.cos(i*2*math.pi/segments),
                   (radius+tube*math.cos(j*2*math.pi/sides))*math.sin(i*2*math.pi/segments),
                   tube*math.sin(j*2*math.pi/sides))
                  for j in range(sides)] for i in range(segments)]
        for i in range(segments):
            ni = (i+1) % segments
            for j in range(sides):
                nj = (j+1) % sides
                points = [local[i][j], local[ni][j], local[ni][nj], local[i][nj]]
                mid = center(points)
                radial = unit((mid[0], mid[1], 0))
                outward = sub(mid, mul(radial, radius))
                transformed = [add(along_axis(p, axis), offset) for p in points]
                self.face(transformed, slot, along_axis(outward, axis))

    def octahedron(self, radii, slot, offset=(0,0,0)):
        rx, ry, rz = radii
        vertices = ((rx,0,0), (0,ry,0), (-rx,0,0), (0,-ry,0),
                    (0,0,rz), (0,0,-rz))
        for pole in (4,5):
            for i in range(4):
                points = [add(vertices[j], offset) for j in (i,(i+1)%4,pole)]
                self.face(points, slot, sub(center(points), offset))

    def sphere(self, radius, slot, segments=10, rings=4):
        """A deliberately faceted sphere, with closed pole caps."""
        circles = []
        for ring in range(1, rings):
            theta = ring * math.pi / rings
            circles.append([(radius * math.sin(theta) * math.cos(i*2*math.pi/segments),
                             radius * math.sin(theta) * math.sin(i*2*math.pi/segments),
                             radius * math.cos(theta)) for i in range(segments)])
        for i in range(segments):
            ni = (i+1) % segments
            for pole, circle in (((0,0,radius), circles[0]),
                                 ((0,0,-radius), circles[-1])):
                points = [pole, circle[i], circle[ni]]
                self.face(points, slot, center(points))
            for a, b in zip(circles, circles[1:]):
                points = [a[i], a[ni], b[ni], b[i]]
                self.face(points, slot, center(points))

    def validate(self):
        assert self.triangles
        assert set(t.slot for t in self.triangles) == set(SLOTS)
        for triangle in self.triangles:
            a, b, c = triangle.points
            geometric = unit(cross(sub(b,a), sub(c,a)))
            assert dot(geometric, triangle.normal) > .999999
            assert abs(length(triangle.normal) - 1) < 1.e-6
            assert all(math.isfinite(x) for p in triangle.points for x in p)
            assert all(-1.e-6 <= x <= 1+1.e-6 for uv in triangle.uv for x in uv)
        vertices = [p for t in self.triangles for p in t.points]
        lower = tuple(min(v[axis] for v in vertices) for axis in range(3))
        upper = tuple(max(v[axis] for v in vertices) for axis in range(3))
        radius = max(length(v) for v in vertices)
        if self.name.startswith('SM_D0'):
            assert radius <= 20 + 1.e-6, (self.name, radius)
        if self.name == 'SM_CoverModule':
            assert lower == (-50.,-50.,-50.) and upper == (50.,50.,50.)
        return dict(bounds_cm=dict(min=[round(v,6) for v in lower],
                                   max=[round(v,6) for v in upper]),
                    size_cm=[round(upper[a]-lower[a],6) for a in range(3)],
                    max_vertex_radius_cm=round(radius,6),
                    vertex_count=len(vertices), triangle_count=len(self.triangles),
                    normal_count=len(self.triangles), uv_count=len(vertices),
                    triangles_per_material={s:sum(t.slot==s for t in self.triangles) for s in SLOTS})

    def obj(self):
        """Flat-shaded triangles; each face owns vertices, normals, and planar UVs."""
        triangles = [t for slot in SLOTS for t in self.triangles if t.slot == slot]
        lines = [f'# Original MothEffect T20 source, generator {VERSION}.',
                 '# Centimeters; +Z up; +X forward; no embedded coordinate transform.',
                 '# Reproduce with scripts/generate_t20_art.py; copy user edits first.',
                 'mtllib M_PrototypeArt.mtl', f'o {self.name}']
        def numbers(values):
            return ' '.join(f'{0. if abs(v)<.0000005 else v:.6f}' for v in values)
        for triangle in triangles:
            lines.extend('v '+numbers(p) for p in triangle.points)
        for triangle in triangles:
            lines.extend('vt '+numbers(uv) for uv in triangle.uv)
        lines.extend('vn '+numbers(t.normal) for t in triangles)
        lines.append('s off')
        previous_slot = None
        for i, triangle in enumerate(triangles):
            if triangle.slot != previous_slot:
                lines.extend((f'g {self.name}_{triangle.slot}', f'usemtl {triangle.slot}'))
                previous_slot = triangle.slot
            indices = range(i*3+1, i*3+4)
            lines.append('f '+' '.join(f'{v}/{v}/{i+1}' for v in indices))
        return ('\n'.join(lines)+'\n').encode('utf-8')


def make_launcher():
    mesh = Mesh('SM_D01Launcher', 'Upward launch pedestal and triangular arrow prism',
                'visual_only; device gameplay sphere remains authoritative')
    mesh.cylinder(11.5, -14, -10, 'Metal')
    mesh.cylinder(11.9, -11, -9.5, 'Copper')
    mesh.box((-4.5,-4.5,-10), (4.5,4.5,5), 'Metal')
    mesh.prism_xz([(-10.5,3), (10.5,3), (0,15.5)], -3.5,3.5,'Metal')
    for y0,y1 in ((-3.9,-3.5),(3.5,3.9)):
        mesh.prism_xz([(-7.5,4), (7.5,4), (0,13.7)], y0,y1,'Glow')
    for x0,x1 in ((-5,-3.7),(3.7,5)):
        mesh.box((x0,-5,-7), (x1,5,-4), 'Copper')
    mesh.box((-1.6,-4.9,-7), (1.6,-4.5,2), 'Glow')
    return mesh


def make_bomb():
    mesh = Mesh('SM_D02Bomb', 'Orange faceted core, crossed sealing rings, copper seal',
                'visual_only; device gameplay sphere remains authoritative')
    mesh.sphere(10.5, 'Glow')
    mesh.torus(13.2, 2.2, 'Metal', axis='Y', segments=12, sides=4)
    mesh.torus(13.2, 1.35, 'Copper', axis='Z', segments=12, sides=4)
    for sign in (-1,1):
        x0,x1 = ((-17,-14) if sign<0 else (14,17))
        mesh.box((x0,-2.8,-3), (x1,2.8,3), 'Metal')
        x0,x1 = ((-17.3,-17) if sign<0 else (17,17.3))
        mesh.box((x0,-2,-2), (x1,2,2), 'Copper')
    mesh.cylinder(3,12.5,15,'Copper')
    return mesh


def make_emitter():
    mesh = Mesh('SM_D03Emitter', 'Directional rectangular body with +X muzzle',
                'visual_only; device gameplay sphere remains authoritative')
    mesh.box((-13,-6,-5), (4,6,5), 'Metal')
    mesh.box((-12,-3,-8), (0,3,-5), 'Metal')
    mesh.cylinder(4.2,4,16,'Metal',axis='X')
    mesh.cylinder(4.7,8,10,'Copper',axis='X')
    mesh.cylinder(2.6,16,16.3,'Glow',axis='X')
    mesh.box((-10,-6.4,-2), (-4,-6,2), 'Copper')
    mesh.box((-10,6,-2), (-4,6.4,2), 'Copper')
    mesh.prism_xz([(-8,5.1), (2,5.1), (2,6.8), (-8,6.8)], -1.1,1.1,'Glow')
    # Forward-pointing side engravings terminate toward the physical +X muzzle.
    for y0,y1 in ((-6.5,-6.4),(6.4,6.5)):
        mesh.prism_xz([(-2,-2.5), (2,0), (-2,2.5)], y0,y1,'Glow')
    return mesh


def make_cover():
    mesh = Mesh('SM_CoverModule', 'Faceted metal barricade, framed feet and luminous chevron',
                'retains original T17 cube collision at exact [-50,50] cm bounds')
    mesh.prism_xz([(-43,-43),(43,-43),(43,31),(31,43),(-31,43),(-43,31)],
                  -34,34,'Metal')
    mesh.box((-50,-50,-50),(50,50,-40),'Metal')
    for x0,x1 in ((-50,-41),(41,50)):
        mesh.box((x0,-41,-40),(x1,41,50),'Metal')
    mesh.box((-50,-41,40),(50,41,50),'Metal')
    for side in (-1,1):
        y0,y1 = ((-40,-34) if side<0 else (34,40))
        for x0,x1 in ((-38,-30),(30,38)):
            mesh.box((x0,y0,-35),(x1,y1,35),'Copper')
        for x0,x1 in ((-47,-39),(39,47)):
            y0,y1 = ((-50,-41) if side<0 else (41,50))
            mesh.box((x0,y0,34),(x1,y1,43),'Copper')
        y0,y1 = ((-35,-34) if side<0 else (34,35))
        for arm in ([(-23,-8),(0,11),(0,18),(-23,-1)],
                    [(0,11),(23,-8),(23,-1),(0,18)]):
            mesh.prism_xz(arm,y0,y1,'Glow')
    return mesh


def make_beacon():
    mesh = Mesh('SM_RuneBeacon', 'Switchable decoration: floating rune crystal and four-post frame',
                'NoCollision; no navigation relevance; actor visibility may be disabled')
    mesh.cylinder(15,0,6,'Metal')
    mesh.cylinder(15.4,5,7,'Copper')
    for x in (-9,9):
        for y in (-9,9):
            mesh.box((x-1.2,y-1.2,6),(x+1.2,y+1.2,44),'Metal')
    mesh.torus(13.2,1.2,'Copper',offset=(0,0,43),segments=8)
    mesh.octahedron((8.5,8.5,15),'Glow',offset=(0,0,27))
    mesh.prism_xz([(-5,8),(0,12),(5,8),(0,4)],-15.5,-14.5,'Copper')
    mesh.prism_xz([(-2.5,8),(0,10),(2.5,8),(0,6)],-15.7,-15.5,'Glow')
    return mesh


MTL = '''# Original MothEffect T20 material hints; no external textures.
# Fixed slot order: Metal, Copper, Glow. Unreal materials may replace these hints.
newmtl Metal
Ka 0.025 0.035 0.050
Kd 0.120 0.160 0.200
Ks 0.550 0.550 0.550
Ns 70
d 1.0
illum 2

newmtl Copper
Ka 0.080 0.040 0.015
Kd 0.580 0.280 0.080
Ks 0.500 0.320 0.160
Ns 55
d 1.0
illum 2

newmtl Glow
Ka 0.000 0.140 0.180
Kd 0.020 0.700 0.900
Ke 0.020 0.700 0.900
Ks 0.100 0.100 0.100
Ns 15
d 1.0
illum 2
'''

README = '''# T20 original prototype art sources

These five low-poly meshes were constructed for MothEffect by
`scripts/generate_t20_art.py` using only Python's standard library. They contain
no downloaded meshes, textures, third-party recordings, or borrowed asset data.
No independent asset license is declared here; the repository currently contains
no LICENSE file. Do not infer a public-domain, Creative Commons, or permissive
license from the assets being code-generated.

Run `python scripts/generate_t20_art.py` to reproduce this directory. This explicit
operation overwrites its own five OBJ files, shared MTL, manifest, and this README.
Copy any user modifications to a different name/location before regenerating.
`python scripts/generate_t20_art.py --check` compares the generated bytes without
writing any files. Unrelated files in this directory are left untouched.

All OBJ coordinates are centimeters, +Z up, +X forward. The legacy Unreal FBX
OBJ importer mirrors +Y to -Y when converting handedness; the import script
validates that conversion and preserves +Z/+X. D01 points upward; D03's muzzle
is at +X. Each device vertex fits
inside the unchanged 20 cm gameplay sphere. The cover has exact [-50,50] cm bounds
on all three axes so T17's existing cube scales and collision can be retained.
The beacon is decoration and should have no collision or navigation relevance.

Material slots always appear in the order `Metal`, `Copper`, `Glow`. The shared
MTL offers preview colors only: runtime material instances should color D01 cyan,
D02 orange, and D03 violet independently. OBJ normals are flat per triangle, and
every triangle has planar UV coordinates. No texture files are required.
Shapes and small copper pieces provide identity beyond color. Several closed
primitive pieces intersect deliberately; these are visual meshes, not gameplay
collision hulls. Replace the visual mesh while retaining the actor's collision
and gameplay components. Manifest bounds, triangle counts, and SHA-256 values are
checked by the generator, including exported triangle winding and normal signs.
'''


def validate_obj(data, mesh):
    """Independently validate the rounded, serialized coordinates and face indices."""
    vertices, normals, uvs, faces, used_slots = [], [], [], [], []
    for line in data.decode('utf-8').splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == 'v':
            vertices.append(tuple(float(x) for x in parts[1:]))
        elif parts[0] == 'vn':
            normals.append(tuple(float(x) for x in parts[1:]))
        elif parts[0] == 'vt':
            uvs.append(tuple(float(x) for x in parts[1:]))
        elif parts[0] == 'usemtl':
            used_slots.append(parts[1])
        elif parts[0] == 'f':
            faces.append([tuple(int(x)-1 for x in index.split('/')) for index in parts[1:]])
    assert tuple(used_slots) == SLOTS
    assert len(vertices) == len(uvs) == 3*len(faces)
    assert len(normals) == len(faces) == len(mesh.triangles)
    for face in faces:
        assert len(face) == 3
        assert all(0 <= i[0] < len(vertices) and 0 <= i[1] < len(uvs)
                   and 0 <= i[2] < len(normals) for i in face)
        a,b,c = [vertices[i[0]] for i in face]
        geometric = unit(cross(sub(b,a),sub(c,a)))
        assert all(dot(geometric,normals[i[2]]) > .99999 for i in face)
    if mesh.name.startswith('SM_D0'):
        assert max(length(v) for v in vertices) <= 20+1.e-6


def generated_files():
    meshes = (make_launcher(),make_bomb(),make_emitter(),make_cover(),make_beacon())
    files = {'M_PrototypeArt.mtl':MTL.encode('utf-8'), 'README.md':README.encode('utf-8')}
    entries = []
    for mesh in meshes:
        statistics = mesh.validate()
        filename = mesh.name+'.obj'
        obj = mesh.obj()
        validate_obj(obj,mesh)
        files[filename] = obj
        entries.append(dict(name=mesh.name,source_file=filename,
                            source_sha256=sha256(obj).hexdigest(),
                            material_slots=list(SLOTS),purpose=mesh.purpose,
                            collision_policy=mesh.collision_policy,**statistics))
    manifest = dict(generator='scripts/generate_t20_art.py',generator_version=VERSION,
                    generator_sha256=sha256(Path(__file__).read_bytes()).hexdigest(),
                    authorship='Original code-generated geometry for YYchainsAw/MothEffect T20',
                    third_party_assets=[],
                    licensing='No separate asset license declared; repository has no LICENSE file.',
                    coordinates=dict(units='centimeters',up='+Z',forward='+X',
                                     embedded_transform='none', unreal_import='Legacy FBX OBJ mirrors Y to convert handedness; X and Z unchanged'),
                    device_constraint='Every vertex within a radius-20 cm sphere at origin',
                    cover_constraint='Exact axis bounds [-50,50] cm; preserve T17 gameplay cube',
                    shading='Flat normals; per-face planar UVs; outward triangle winding',
                    material_slots=list(SLOTS),shared_material_file='M_PrototypeArt.mtl',
                    shared_material_sha256=sha256(files['M_PrototypeArt.mtl']).hexdigest(),
                    regeneration='Explicit run overwrites only generated files; copy user edits first',
                    meshes=entries)
    files['PrototypeArtManifest.json'] = (json.dumps(manifest,indent=2)+'\n').encode('utf-8')
    return files,entries


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true',help='Read-only byte and geometry verification')
    args = parser.parse_args()
    files,entries = generated_files()
    if args.check:
        mismatches = [name for name,data in files.items()
                      if not (OUT/name).is_file() or (OUT/name).read_bytes()!=data]
        if mismatches:
            raise SystemExit('Generated sources differ: '+', '.join(mismatches))
        print(f'Verified {len(entries)} original meshes and {len(files)} deterministic files.')
    else:
        print('Regenerating owned T20 files; copy user edits elsewhere before this explicit run.')
        OUT.mkdir(parents=True,exist_ok=True)
        for name,data in files.items():
            (OUT/name).write_bytes(data)
        print(f'Generated {len(entries)} original meshes in {OUT}')
    for entry in entries:
        print(f"{entry['name']}: {entry['triangle_count']} triangles; "
              f"bounds={entry['bounds_cm']}; radius={entry['max_vertex_radius_cm']} cm")


if __name__ == '__main__':
    main()
