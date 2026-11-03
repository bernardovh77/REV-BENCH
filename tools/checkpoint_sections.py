"""Declared, hashed sections and diagnostic assets for the single offline result.

Sections contain already prepared listening previews. Republishing requires no
analysis packages or rerunning simulations, and never edits measurement audio.
"""
import hashlib
import json
from pathlib import Path
import shutil
import tempfile

SECTIONS = {
    "audio-experiment": ("AUDIO EXPERIMENT", "Research results"),
    "transport-candidates": ("ACOUSTIC CANDIDATES", "Renderer prototypes"),
    "user-references": ("USER REFERENCES", "New references"),
    "coupling-proof": ("COUPLING PROOF", "Coupling proof"),
    "reference-protocol": ("REFERENCE PROTOCOL", "Reference controls"),
    "geometry-checkpoint": ("GEOMETRY CHECKPOINT", "Stages 43–44"),
    "flow-checkpoint": ("FLOW CHECKPOINT", "Stages 45–46"),
    "thermal-checkpoint": ("THERMAL CHECKPOINT", "Stages 47–48"),
    "fixed-port-checkpoint": ("FIXED PORT CHECKPOINT", "Stage 114"),
    "junction-checkpoint": ("JUNCTION CHECKPOINT", "Stage 49"),
}


def fingerprint(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def relative_file(root, name):
    path = root / name
    if Path(name).is_absolute() or '..' in Path(name).parts or path.is_symlink():
        raise ValueError("Checkpoint asset must be a local regular file")
    if not path.resolve().is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f"Missing checkpoint asset: {name}")
    return path


def load(renders):
    path = renders / "checkpoint/manifest.json"
    if not path.exists():
        return {"schema": 1, "sections": [], "diagnostics": []}
    result = json.loads(path.read_text(encoding='utf-8'))
    if result.get("schema") != 1:
        raise ValueError("Unsupported checkpoint schema")
    ids = [entry["id"] for entry in result["sections"]]
    names = [entry["path"] for entry in result["diagnostics"]]
    if len(ids) != len(set(ids)) or len(names) != len(set(names)):
        raise ValueError("Duplicate checkpoint section or diagnostic")
    return result


def checked(root, entry):
    path = relative_file(root, entry["path"])
    if fingerprint(path) != entry["sha256"]:
        raise ValueError(f"Checkpoint fingerprint differs: {entry['path']}")
    return path


def save(renders, manifest):
    path = renders / "checkpoint/manifest.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.pending.json')
    try:
        temporary.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='')
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def retained_diagnostics(renders, folder=None):
    """The fingerprints of the retained diagnostic files: renders/current/
    experiment's, or those of `folder` as they will be once it takes its place.
    Names are relative to renders/current, with forward slashes on every
    system."""
    current = renders / 'current'
    folder = current / 'experiment' if folder is None else folder
    result = []
    for path in sorted(folder.rglob('*')):
        if path.is_file():
            name = 'experiment/' + path.relative_to(folder).as_posix()
            if path.is_symlink():
                raise ValueError("Checkpoint asset must be a local regular file")
            result.append(dict(path=name, sha256=fingerprint(path)))
    return result


def staged_experiment(renders, staged):
    """The next renders/current/experiment, built beside it: the retained files
    with the staged ones added or replacing them. Nothing retained changes."""
    current = renders / 'current'
    target = current / '.experiment-next'
    if target.exists():
        shutil.rmtree(target)
    if (current / 'experiment').exists():
        shutil.copytree(current / 'experiment', target)
    else:
        target.mkdir(parents=True)
    for path in sorted(staged.rglob('*')):
        if path.is_file():
            destination = target / path.relative_to(staged)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, destination)
    return target


def adopt_current(renders):
    """One-time migration of known sections and retained diagnostic inventory."""
    if (renders / "checkpoint/manifest.json").exists():
        raise ValueError("Checkpoint already registered; use normal republication")
    page = (renders / "RESULTS.html").read_text(encoding='utf-8')
    manifest = load(renders)
    for key, (marker, label) in SECTIONS.items():
        begin, end = f'<!-- {marker} BEGIN -->', f'<!-- {marker} END -->'
        if begin not in page and end not in page:
            continue
        if page.count(begin) != 1 or page.count(end) != 1:
            raise ValueError(f"Malformed checkpoint section: {key}")
        fragment = begin + page.split(begin, 1)[1].split(end, 1)[0] + end
        destination = renders / f"checkpoint/sections/{key}.html"
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(fragment, encoding='utf-8', newline='')
        manifest['sections'].append(dict(id=key, marker=marker, label=label,
            path=destination.relative_to(renders).as_posix(), sha256=fingerprint(destination)))
    manifest['diagnostics']=retained_diagnostics(renders)
    save(renders, manifest)
    return manifest


def compose(page, renders, manifest=None):
    manifest = load(renders) if manifest is None else manifest
    if page.count('</main>') != 1 or page.count('</nav>') != 1:
        raise ValueError("Expected one results main and navigation element")
    for entry in manifest['sections']:
        fragment = checked(renders, entry).read_text(encoding='utf-8')
        begin, end = f"<!-- {entry['marker']} BEGIN -->", f"<!-- {entry['marker']} END -->"
        if fragment.count(begin) != 1 or fragment.count(end) != 1:
            raise ValueError("Malformed registered section")
        if begin in page or end in page:
            if page.count(begin) != 1 or page.count(end) != 1:
                raise ValueError("Malformed existing section")
            before, rest = page.split(begin, 1)
            page = before + rest.split(end, 1)[1].removeprefix('\n')
        link = f'<a href="#{entry["id"]}">{entry["label"]}</a>'
        page = page.replace(link, '')
        page = page.replace('</main>', fragment + '\n</main>', 1)
        page = page.replace('</nav>', link + '</nav>', 1)
    return page


def diagnostic_names(current, renders):
    manifest = load(renders)
    for entry in manifest['diagnostics']:
        checked(current, entry)
    return {entry['path'] for entry in manifest['diagnostics'] if entry['path'].endswith('.wav')}


def carry_diagnostics(current, staged, renders):
    for entry in load(renders)['diagnostics']:
        source = checked(current, entry)
        target = staged / entry['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            if fingerprint(target) != entry['sha256']:
                raise ValueError("Conflicting staged diagnostic fingerprint")
        else:
            shutil.copyfile(source, target)


def publish_section(target, key, fragment, diagnostics=None):
    """Publish immutable fragments; serialize calls from section producers.

    Ordinary write failures preserve the previous rebuildable checkpoint. The
    page and manifest are separate atomic replacements, not a crash transaction.

    diagnostics (audio-experiment only): a folder of new diagnostic files for
    renders/current/experiment. They join or replace the retained ones only
    together with the page and the manifest: until then the retained files
    keep the fingerprints the manifest has, and a failure puts back both, so
    a failed publish never blocks make results or make render (stage 119).
    """
    if diagnostics is not None and key != 'audio-experiment':
        raise ValueError("Only the audio experiment retains diagnostic files")
    renders = target.parent
    if not (renders / 'checkpoint/manifest.json').exists():
        adopt_current(renders)
    previous_manifest = load(renders)
    manifest = dict(previous_manifest)
    marker, label = SECTIONS[key]
    digest = hashlib.sha256(fragment.encode()).hexdigest()
    destination = renders / f'checkpoint/sections/{key}-{digest}.html'
    destination.parent.mkdir(parents=True, exist_ok=True)
    entry = dict(id=key, marker=marker, label=label,
                 path=destination.relative_to(renders).as_posix(), sha256=digest)
    if destination.exists():
        checked(renders, entry)
    else:
        with tempfile.TemporaryDirectory(dir=destination.parent) as staging:
            staged = Path(staging) / 'section.html'
            staged.write_text(fragment, encoding='utf-8', newline='')
            checked(staged.parent, dict(path=staged.name, sha256=digest))
            staged.replace(destination)
    manifest['sections'] = [row for row in manifest['sections'] if row['id'] != key] + [entry]
    experiment = renders / 'current/experiment'
    following = previous = None
    if diagnostics is not None:
        following = staged_experiment(renders, Path(diagnostics))
        previous = renders / 'current/.experiment-previous'
        if previous.exists():
            shutil.rmtree(previous)
        manifest['diagnostics'] = retained_diagnostics(renders, following)
    elif key == 'audio-experiment':
        manifest['diagnostics'] = retained_diagnostics(renders)
    temporary = target.with_suffix('.checkpoint.pending.html')
    swapped = False
    try:
        temporary.write_text(compose(target.read_text(encoding='utf-8'), renders, manifest),
                             encoding='utf-8', newline='')
        if following is not None:
            # The files, then the manifest that names them, then the page.
            if experiment.exists():
                experiment.rename(previous)
            following.rename(experiment)
            swapped = True
        save(renders, manifest)
        try:
            temporary.replace(target)
        except BaseException:
            save(renders, previous_manifest)
            raise
    except BaseException:
        if swapped:
            experiment.rename(following)
            if previous.exists():
                previous.rename(experiment)
        raise
    finally:
        temporary.unlink(missing_ok=True)
        for leftover in (following, previous):
            if leftover is not None and leftover.exists():
                shutil.rmtree(leftover)
