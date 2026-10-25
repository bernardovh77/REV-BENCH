# REV-BENCH

REV-BENCH models an engine, its gas exchange and its audible response to physical
changes. These terms distinguish the engine, the sound-producing processes and
the evidence used to judge them.

## Language

**Prepared exhaust geometry**:
A validated, immutable description compiled into physical regions, port and
cell bindings, finite volumes and source/outlet paths. Stage044 executes a
lossless geometry control; preparation does not qualify complete muffler,
catalyst, hot-flow or radiation behavior.

**Pressure-equilibrium preservation**:
Keeping initially uniform pressure and velocity uniform while a source-free
gas-mixture contact moves. Conserving mass and total energy alone does not
guarantee it; Stage043's rejected candidate demonstrates the distinction.

**Ambient terminal identity**:
The identity of one physical aperture into the environment. Two outlets may
share ambient temperature and pressure without a zero-delay acoustic connection.

**Physical engine**:
The engine whose crank motion, combustion, valves and gas exchange determine its
operating response and the causes of its sound.
_Avoid_: AI engine, sound generator when referring to the whole engine

**Excitation**:
A physical disturbance that produces sound, such as a combustion pulse,
turbulent jet or mechanical impact.
_Avoid_: exhaust tone when referring to the disturbance before propagation

**Actual spark**:
The physical discharge event, distinct from the engine controller's commanded
spark timing and from subsequent flame development or heat release.
_Avoid_: ignition when that would conflate the command, discharge and flame

**Kernel development**:
The interval after actual spark discharge before the modeled bulk flame release.
The current Stage040 engine gives it a nonnegative development clock; its delay
surrogate is not an experimentally fitted extinction or misfire law.
_Avoid_: spark jitter when referring to post-discharge flame development

**Conditional mean burn**:
The expected modeled duration and development delay for the current trapped
charge, distinct from this cycle's random draw. Variation amplitude changes
the spread continuously; it does not select a different mean-model policy.
_Avoid_: deterministic when that incorrectly implies every physical cycle repeats

**Burned-product mass fraction**:
The mass share labeled burned gas in the current species model. Retained
unburned fuel and air still affect mixture and energy, but there is no tracer
identifying the total mass carried from an earlier engine cycle.
_Avoid_: measured total residual-gas fraction

**Gross and net IMEP**:
Cylinder work per swept volume over a stated complete cycle. Gross covers
compression/expansion; net also includes gas exchange. Their variation must
use matching cylinder windows; a small net mean can magnify percentage COV.
_Avoid_: combustion instability when inferred only from net-work COV

**Acoustic propagation**:
The travel, reflection, interference and attenuation of sound through an engine
system and its surroundings.
_Avoid_: timbre filter when referring to physical wave transport

**Mean gas state**:
The background gas conditions and inventories about which acoustic disturbances
are distinguished, including flow, pressure, temperature and composition.
_Avoid_: temperature setting when the transported state is intended

**Gas/acoustic interface**:
The physical boundary at which gas transport and acoustic disturbances exchange
load and energy between complementary descriptions of an exhaust region.
_Avoid_: audio output connection

**Port**:
A physical connection at which pressure and oriented flow connect two components.
_Avoid_: outlet when the connection is internal

**Reflected load**:
The returning pressure disturbance through which the exhaust influences the
upstream gas exchange and engine response.
_Avoid_: echo when the effect on the engine is intended

**Radiation load**:
The acoustic load exerted by the surroundings on an opening.
_Avoid_: listener filter

**Radiated sound**:
Sound carried away from an opening or vibrating structure to a listener.
_Avoid_: radiation load

**Reference recording**:
A real recording used as observational evidence, with its recording conditions
and uncertainty attached.
_Avoid_: calibrated target unless the required conditions are established

**Numerical validation**:
Evidence that a stated model solves its declared physical problem accurately
within a specified operating range.
_Avoid_: realism acceptance

**Realism acceptance**:
Evidence that the engine's sound and response agree sufficiently with real
examples, including Bernardo's listening judgment.
_Avoid_: passing numerical tests

**Engine build**:
A chosen engine, intake, exhaust and tuning configuration, distinct from the
engine's changing running state.
_Avoid_: snapshot when configuration alone is intended

**Validated operating envelope**:
The engine configurations and operating conditions for which a model's stated
accuracy and behavior have supporting evidence.
_Avoid_: allowed slider range when physical credibility is intended

**Fidelity tier**:
A declared choice of physical and acoustic resolution with an identified
supported range and processing demand.
_Avoid_: quality when it implies an unmeasured realism ranking

**Visual state**:
A coherent observation of the engine and its actual events at an identified
engine time, used to depict its running condition.
_Avoid_: animation state when unrelated motion is intended

**Diagnostic time**:
Engine time deliberately presented more slowly or by controlled steps for
inspection, distinct from normal live operation.
_Avoid_: slow motion when only unrelated cosmetic movement is slowed

**Illustrative visualization**:
A labeled depiction that explains an engine process without claiming a
spatially resolved physical result, such as a flame shape from lumped burn fraction.
_Avoid_: CFD result or resolved flame front without that physical solution

**Build revision**:
The immutable configuration version, distinct from the running-state generation.
A cold reset may keep this revision while replacing all running state.
_Avoid_: generation when referring only to a tuning/configuration edit

**Prepared-state generation**:
The identity of one installed engine state, its compiled topology, event history
and observation epoch. Rebuild/reset invalidates old handles and observations.
_Avoid_: pipe index or native pointer as a persistent identity

**Logical boundary graph**:
Component interfaces and their semantic connections, distinct from a solver's
cells/faces and from the spatial centerline used to depict the physical route.
_Avoid_: mesh when referring to logical component connectivity

## Numerical observation (Stage038)

- **Requested/accepted transfer:** signed kg, species kg and transported total
  enthalpy J before/after an existing transport limit. Foregone transfer is
  distinct from energy removed from a control volume.
- **Staggered-column impulse:** Δmdot times effective momentum length, in N·s;
  requested/accepted values expose sonic/reset intervention. It does not give
  a 0D junction a vector-momentum state.
- **Diagnostic slot:** caller-owned aggregate identified by semantic component
  role/number and cell/face part; binding is quiescent and must be repeated
  after rebuild/reset. A bounded trace can drop records while aggregates persist.

See [Stage038](docs/stage-038.md) for coverage, cost and limitations.

## Delivered-fuel event

The engine-owned record of an actual positive fuel mass and its sensible-energy
addition, with cylinder, charge, tick and substep time. The current IVC charge
fueling is a collapsed surrogate, not a resolved PFI/GDI needle schedule.
Only its delivered events excite the modeled injector source. Skipped fueling
(retained fuel sufficient, disabled request or DFCO) creates no new excitation;
existing acoustic ringing may continue.

## Fueling attempt

One decision at intake closing, recording actual delivery or an explicit skip
reason. It distinguishes fuel already in the cylinder from new injected fuel.
A spark suppression event is a separate decision and can occur after a real dose.


## Uniform-flow acoustic coupling (Stages045–046)

- **Acoustic mass flow:** the perturbation in transported mass, including both
  density times volume-flow change and mean velocity times density change.
  It differs from acoustic volume flow when the gas has a nonzero mean speed.
- **Contact coordinates:** retained entropy/composition degrees of freedom
  transported at the gas velocity, alongside pressure waves traveling at U±c.
- **Coordinate work:** change in a quadratic wave-energy diagnostic caused by
  inserting the conservative nonlinear residual into the same physical state.
  It is an account of that representation, never extra gas energy.
- **Frozen mean flow:** a uniform reference speed fixed during one experiment.
  It does not establish evolving hot/cold fronts or time-dependent reversal.


## Thermal/material reference (Stages047–048)

- **Finite wall capacity:** solid energy Cw*Tw in joules, separate from the gas.
  Its spatial interval stays fixed as gas moves; gas refinement does not add metal.
- **Material cell:** a parcel with fixed species masses and evolving volume and
  sensible internal energy. Its boundaries move with gas, unlike a fixed engine
  port. A material reference does not automatically supply fixed-port mass flow.
- **Compatible pressure work:** accepted midpoint pressure multiplies volume
  change from the same midpoint velocities used by the momentum equation.
  This ties gas internal energy and nodal kinetic work to one discrete account.
- **Ambient entrainment:** actual outside gas crosses into the duct. Negative
  velocity can instead return gas that previously left, so direction alone is
  not proof that cold ambient composition entered.
- **Packet fidelity:** accuracy of the complete transient waveform/envelope,
  beyond magnitude/phase at a single carrier frequency.
