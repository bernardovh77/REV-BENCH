"""Stage 037 executable contract reference, outside production/runtime code.

JSON validation is structural. Native descriptor checks and preparation remain
mandatory before engine use; this module is not a solver or geometry compiler.
"""
import copy
import json
import math
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
KINDS = ('runner', 'header', 'exhaust', 'tailpipe', 'muffler', 'snorkel', 'center', 'link')
PLACES = ('ambient', 'plenum', 'intake-valve', 'exhaust-valve', 'collector',
          'muffler-inlet', 'muffler-outlet', 'airbox', 'center-inlet', 'center-outlet', 'junction')
MAX_JSON = 1024 * 1024


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def keys(obj, expected):
    require(isinstance(obj, dict) and set(obj) == set(expected.split()), 'Unsupported/missing fields')


def number(value):
    try:
        finite = type(value) in (int, float) and math.isfinite(value)
    except OverflowError:
        finite = False
    require(finite, 'Finite number required')
    return value


def counter(value):
    require(isinstance(value, str) and re.fullmatch(r'0|[1-9][0-9]{0,19}', value) is not None,
            'Canonical uint64 decimal string required')
    require(int(value) < 2**64, 'Counter overflow')
    return int(value)


def loads(text):
    require(len(text.encode('utf-8')) <= MAX_JSON, 'Build exceeds 1 MiB')
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, 'Duplicate JSON key')
            result[key] = value
        return result
    return json.loads(text, object_pairs_hook=unique,
                      parse_constant=lambda _: require(False, 'Nonfinite JSON number'))


def dumps(build):
    validate_build(build)
    text = json.dumps(build, sort_keys=True, ensure_ascii=True, allow_nan=False, separators=(',', ':'))
    require(len(text.encode()) <= MAX_JSON, 'Build exceeds 1 MiB')
    return text


def catalog():
    return json.loads((ROOT / 'contracts/engine-fields-v1.json').read_text(encoding='utf-8'))


def from_native(raw, generation='1'):
    """Observe the existing logical pipe API; do not invent physical centerlines."""
    namespace = raw['preset']
    components, nodes, sources = [], {}, []
    for p in raw['pipes']:
        for name in ('number', 'from_number', 'to_number'):
            require(type(p[name]) is int and p[name] >= 0, 'Nonnegative native identity index required')
        require(type(p['kind']) is int and 0 <= p['kind'] < len(KINDS), 'Unsupported native pipe kind')
        kind = KINDS[p['kind']]
        suffix = 'snorkel' if kind == 'snorkel' else f'{kind}-{p["number"]}'
        cid = f'{namespace}/{suffix}'
        ports = []
        for end, direction in ((0, 'from'), (1, 'to')):
            require(type(p[direction]) is int and 0 <= p[direction] < len(PLACES), 'Unsupported native place')
            place = PLACES[p[direction]]
            if place == 'ambient':
                place = 'ambient-intake' if kind == 'snorkel' else 'ambient-exhaust'
            nid = f'{namespace}/{place}-{p[direction + "_number"]}'
            nodes[nid] = dict(id=nid, kind=place)
            ports.append(dict(id=f'{cid}:{end}', node_id=nid, axis_sign=1 if end == 0 else -1))
        components.append(dict(id=cid, kind=kind, length_m=p['length'],
                               area_m2=math.pi*p['diameter']**2/4,
                               ports=ports, centerline_m=None))
        if kind in ('tailpipe', 'snorkel'):
            end = 1 if kind == 'tailpipe' else 0
            sources.append(dict(id=f'{cid}/radiation', component_id=cid, port_id=f'{cid}:{end}',
                                kind='aperture', implemented=False))
    build = dict(schema_version=1, kind='engine-build', build_id=namespace, generation=generation, revision='1',
                 parameters=raw['parameters'],
                 geometry=dict(representation='legacy-logical', frame='engine', engine_to_vehicle=None,
                               components=components, nodes=list(nodes.values())),
                 sources=sources, required_features=[], extensions={})
    validate_build(build)
    return build


def validate_build(build, require_spatial=False):
    keys(build, 'schema_version kind build_id generation revision parameters geometry sources required_features extensions')
    require(type(build['schema_version']) is int and build['schema_version'] == 1 and
            build['kind'] == 'engine-build', 'Unsupported build version/kind')
    require(isinstance(build['build_id'], str) and
            re.fullmatch(r'[a-z][a-z0-9-]{0,63}', build['build_id']) is not None, 'Invalid build ID')
    counter(build['generation'])
    counter(build['revision'])
    require(build['required_features'] == [], 'Unsupported required feature')
    require(isinstance(build['extensions'], dict) and all(
        re.fullmatch(r'x-[a-z0-9-]+', k) for k in build['extensions']), 'Namespaced annotations only')
    # Annotations may round-trip, but never secretly activate physical features.
    json.dumps(build['extensions'], allow_nan=False)
    fields, parameters = catalog(), build['parameters']
    require(isinstance(parameters, dict) and set(parameters) == set(fields), 'Engine field set differs')
    for name in ('cylinders', 'banks'):
        v = parameters[name]
        require(isinstance(v, list) and len(v) == 1 and type(v[0]) is int, 'Integer layout count required')
    cylinders, banks = parameters['cylinders'][0], parameters['banks'][0]
    require(1 <= cylinders <= 16 and 1 <= banks <= 2, 'Unsupported layout capacity')
    for name, field in fields.items():
        expected = {'scalar': 1, 'bank': banks, 'cylinder': cylinders}[field['shape']]
        values = parameters[name]
        require(isinstance(values, list) and len(values) == expected, 'Field shape differs')
        for v in values:
            number(v)
            require(not field['integer'] or type(v) is int, 'Integer engine field required')
    geometry = build['geometry']
    keys(geometry, 'representation frame engine_to_vehicle components nodes')
    require(geometry['representation'] == 'legacy-logical' and geometry['frame'] == 'engine',
            'Unsupported geometry representation/frame')
    transform = geometry['engine_to_vehicle']
    if transform is not None:
        keys(transform, 'translation_m rotation_xyzw')
        vector(transform['translation_m'], 3)
        vector(transform['rotation_xyzw'], 4)
        require(abs(sum(x*x for x in transform['rotation_xyzw'])-1) <= 1e-10, 'Unit quaternion required')
    require(not require_spatial or transform is not None, 'Unresolved engine placement')
    require(isinstance(geometry['nodes'], list) and 1 <= len(geometry['nodes']) <= 256, 'Node capacity')
    node_ids = set()
    for node in geometry['nodes']:
        keys(node, 'id kind')
        require(node['kind'] in PLACES[1:]+('ambient-intake', 'ambient-exhaust'), 'Unsupported node')
        identity(node['id'], build, node_ids)
    components, component_ids, port_ids = geometry['components'], set(), set()
    require(isinstance(components, list) and 1 <= len(components) <= 128, 'Component capacity')
    attachments = set()
    for component in components:
        keys(component, 'id kind length_m area_m2 ports centerline_m')
        identity(component['id'], build, component_ids)
        require(component['kind'] in KINDS, 'Unsupported component')
        require(0 < number(component['length_m']) <= 100 and 0 < number(component['area_m2']) <= 10,
                'Invalid component dimensions')
        require(isinstance(component['ports'], list) and len(component['ports']) == 2, 'Two-port legacy component required')
        for end, port in enumerate(component['ports']):
            keys(port, 'id node_id axis_sign')
            identity(port['id'], build, port_ids)
            require(port['id'] == f'{component["id"]}:{end}' and
                    type(port['axis_sign']) is int and port['axis_sign'] == (1 if end == 0 else -1), 'Port orientation differs')
            require(port['node_id'] in node_ids, 'Dangling port')
            attachments.add(port['node_id'])
        path = component['centerline_m']
        if path is not None:
            require(isinstance(path, list) and 2 <= len(path) <= 128, 'Centerline capacity')
            for point in path:
                vector(point, 3)
            length = sum(math.dist(a, b) for a, b in zip(path, path[1:]))
            require(math.isclose(length, component['length_m'], rel_tol=1e-8, abs_tol=1e-10), 'Centerline changes acoustic length')
        require(not require_spatial or path is not None, 'Unresolved centerline')
    require(attachments == node_ids, 'Unused logical node')
    require(isinstance(build['sources'], list) and len(build['sources']) <= 64, 'Source capacity')
    source_ids = set()
    for source in build['sources']:
        keys(source, 'id component_id port_id kind implemented')
        identity(source['id'], build, source_ids)
        require(source['component_id'] in component_ids and source['port_id'] in port_ids and
                source['port_id'].startswith(source['component_id']+':'), 'Dangling source')
        require(source['kind'] == 'aperture' and source['implemented'] is False,
                'Unqualified source binding')
    return build


def vector(v, count):
    require(isinstance(v, list) and len(v) == count, 'Vector shape')
    for x in v:
        number(x)


def identity(value, build, used):
    require(isinstance(value, str) and value.startswith(build['build_id']+'/') and
            re.fullmatch(r'[a-z0-9/:_-]{1,160}', value) is not None and value not in used, 'Duplicate/invalid identity')
    used.add(value)


def validate_snapshot(snapshot, build, previous=None):
    keys(snapshot, 'schema_version build_id generation build_revision sequence physics_tick physics_hz physics_origin audio_frame audio_hz audio_origin cycle_index phase_rad speed_rad_s cylinders')
    require(type(snapshot['schema_version']) is int and snapshot['schema_version'] == 1, 'Unsupported snapshot version')
    require(snapshot['build_id'] == build['build_id'] and snapshot['generation'] == build['generation'] and
            snapshot['build_revision'] == build['revision'], 'Incompatible generation/build revision')
    seq, tick = counter(snapshot['sequence']), counter(snapshot['physics_tick'])
    counter(snapshot['audio_frame']); counter(snapshot['cycle_index'])
    for key in ('physics_hz', 'audio_hz'):
        require(type(snapshot[key]) is int and 8000 <= snapshot[key] <= 1000000, 'Unsupported clock rate')
    pt, af = counter(snapshot['physics_origin']), counter(snapshot['audio_origin'])
    frame = counter(snapshot['audio_frame'])
    require(tick >= pt and frame >= af and
            (tick-pt)*snapshot['audio_hz'] == (frame-af)*snapshot['physics_hz'],
            'Inconsistent simulation/audio boundary')
    require(0 <= number(snapshot['phase_rad']) < 4*math.pi and number(snapshot['speed_rad_s']) >= 0,
            'Invalid forward crank state')
    rows = snapshot['cylinders']
    require(isinstance(rows, list) and len(rows) == build['parameters']['cylinders'][0], 'Incomplete cylinder snapshot')
    for row in rows:
        keys(row, 'pressure_Pa temperature_K burn_fraction')
        require(number(row['pressure_Pa']) > 0 and number(row['temperature_K']) > 0 and
                0 <= number(row['burn_fraction']) <= 1, 'Invalid cylinder state')
    if previous is not None:
        require(previous['generation'] == snapshot['generation'] and previous['build_id'] == snapshot['build_id'] and
                previous['build_revision'] == snapshot['build_revision'], 'History generation/revision differs')
        require(seq > counter(previous['sequence']) and tick >= counter(previous['physics_tick']) and
                counter(snapshot['audio_frame']) >= counter(previous['audio_frame']) and
                snapshot['physics_hz'] == previous['physics_hz'] and snapshot['audio_hz'] == previous['audio_hz'],
                'Stale snapshot or changed clocks')
        require(snapshot['physics_origin'] == previous['physics_origin'] and snapshot['audio_origin'] == previous['audio_origin'] and
                (counter(snapshot['cycle_index']), snapshot['phase_rad']) >=
                (counter(previous['cycle_index']), previous['phase_rad']), 'Changed epoch or backwards crank')
    return snapshot


def validate_event(event, build):
    keys(event, 'schema_version build_id generation build_revision sequence kind actor_id cycle_id physics_tick subtick cycle_index phase_rad')
    require(type(event['schema_version']) is int and event['schema_version'] == 1 and
            event['build_id'] == build['build_id'] and event['generation'] == build['generation'] and
            event['build_revision'] == build['revision'],
            'Incompatible event version/generation')
    for name in ('sequence', 'physics_tick', 'cycle_index', 'cycle_id'):
        counter(event[name])
    require(0 <= number(event['subtick']) < 1 and 0 <= number(event['phase_rad']) < 4*math.pi,
            'Invalid event clock')
    require(event['kind'] in ('spark-command', 'spark-discharge', 'flame-start', 'fuel-injection',
                             'intake-open', 'exhaust-open', 'fuel-cut', 'limiter-cut'), 'Unsupported event')
    actors = {f'{build["build_id"]}/cylinder-{i}' for i in range(build['parameters']['cylinders'][0])}
    require(event['actor_id'] in actors, 'Unknown event actor')
    return event


def audible_performance_time(emission_context_s, gas_flight_s, listener_flight_s,
                             dsp_delay_s, output_context_s, output_performance_s):
    """Static-path oracle; output timestamp already includes output queue/device.
    This does not compute gas/scene flight or claim measured browser accuracy.
    """
    values = (emission_context_s, gas_flight_s, listener_flight_s, dsp_delay_s,
              output_context_s, output_performance_s)
    for x in values:
        require(number(x) >= 0, 'Negative clock/delay')
    return output_performance_s + emission_context_s + gas_flight_s + listener_flight_s + dsp_delay_s - output_context_s


class ReferenceSession:
    """Serial control-plane transaction model, NOT an audio-thread implementation."""
    def __init__(self, build):
        self.active = loads(dumps(build))
        self.last_snapshot = None

    def commit(self, candidate, expected_generation):
        require(expected_generation == self.active['generation'], 'Stale transaction')
        prepared = loads(dumps(candidate))
        require(prepared['build_id'] == self.active['build_id'] and
                counter(prepared['generation']) == counter(expected_generation)+1,
                'Invalid next generation')
        changed = any(prepared[k] != self.active[k] for k in ('parameters', 'geometry', 'sources', 'required_features'))
        require(counter(prepared['revision']) == counter(self.active['revision']) + int(changed),
                'Invalid build revision')
        self.active = prepared
        self.last_snapshot = None

    def accept_snapshot(self, snapshot):
        validate_snapshot(snapshot, self.active, self.last_snapshot)
        self.last_snapshot = copy.deepcopy(snapshot)
