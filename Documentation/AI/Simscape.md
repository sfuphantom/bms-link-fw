---
name: simulink-simscape
description: How to build, configure, run, debug, and analyze models in MATLAB Simulink and Simscape (including Simscape Electrical, Driveline, Fluids, Multibody, and the Simscape language). Use this skill whenever the user mentions Simulink, Simscape, .slx/.mdl/.ssc files, block diagrams, physical modeling, plant models, solver configuration, scopes, Simulink-PS converters, programmatic model construction with add_block/add_line, simulation errors (algebraic loops, DAE/index problems, solver failures), or wants to model circuits, mechanical systems, hydraulics, thermal systems, motors, or control loops in MATLAB, even if they do not name the products explicitly.
---

# Simulink and Simscape

Guidance for helping a user build, run, and debug Simulink (signal-flow) and Simscape (physical-network) models, either through the GUI or by scripting in MATLAB.

## 1. Pick the right modeling domain first

| Situation | Use |
|---|---|
| Control logic, signal processing, state machines, math equations with a clear input → output direction | **Simulink** (signal-flow blocks) |
| Physical components that exchange energy (circuits, springs/masses, hydraulics, heat flow, motors) | **Simscape** (acausal physical networks) |
| Controller driving a physical plant | **Both**: Simulink controller + Simscape plant, joined by converter blocks |
| Custom physical component not in the libraries | **Simscape language** (`.ssc` file) |
| 3D rigid bodies, joints, contacts | **Simscape Multibody** |

Key conceptual difference: Simulink lines carry a **signal with a direction**. Simscape lines are **physical connections** carrying a pair of variables (e.g., voltage and current; force and velocity; pressure and flow rate). Physical connections have no direction and can branch freely, with conservation laws (Kirchhoff, Newton) applied automatically at nodes.

## 2. Core Simulink workflow

1. Create/open the model: `new_system('mdl'); open_system('mdl')` or File → New → Blank Model.
2. Add blocks from the Library Browser (`slLibraryBrowser`) or by script.
3. Connect output ports to input ports.
4. Set parameters (block dialogs or `set_param`).
5. Configure the solver and stop time (Modeling tab → Model Settings).
6. Run (`sim`) and inspect results (Scope, Data Inspector, logged signals).

Essential Simulink blocks and their library paths:

| Purpose | Path |
|---|---|
| Step, Sine Wave, Constant, Ramp | `simulink/Sources/...` |
| Gain, Sum, Product | `simulink/Math Operations/...` |
| Integrator, Transfer Fcn, State-Space | `simulink/Continuous/...` |
| Unit Delay, Discrete Transfer Fcn | `simulink/Discrete/...` |
| Mux, Demux, Bus Creator, Goto/From | `simulink/Signal Routing/...` |
| Scope, To Workspace, Display | `simulink/Sinks/...` |
| Saturation, Lookup Table | `simulink/Discontinuities/...`, `simulink/Lookup Tables/...` |
| Subsystem | `simulink/Ports & Subsystems/Subsystem` |
| MATLAB Function | `simulink/User-Defined Functions/MATLAB Function` |

If a path fails, do not guess further: find the real one (see §8, "Discovering paths and parameters").

## 3. Core Simscape workflow

Every Simscape model needs three things:

1. **Physical components** connected into a network.
2. A **Solver Configuration** block (`nesl_utility/Solver Configuration`) attached anywhere on **each** physical network. One per network; missing it is the most common first error.
3. A **reference/ground** appropriate to the domain (Electrical Reference, Mechanical Translational Reference, Mechanical Rotational Reference, Thermal Reference, Hydraulic Reference, etc.). Missing references cause "floating node" / singular-system errors.

To cross between Simulink and Simscape:

- **Simulink-PS Converter** (`nesl_utility/Simulink-PS Converter`): Simulink signal → physical signal. Set the **Input signal unit** (e.g., `V`, `N`, `rpm`).
- **PS-Simulink Converter** (`nesl_utility/PS-Simulink Converter`): physical signal → Simulink signal. Set the **Output signal unit**.
- Use *Controlled* sources (Controlled Voltage Source, Force Actuator, etc.) to inject a signal into the network. Use *Sensors* (Voltage Sensor, Current Sensor, Ideal Translational Motion Sensor, etc.) to read values out.

Typical Foundation library locations (library prefix `fl_lib`; verify with §8 since layouts can differ by release):

| Domain | Elements |
|---|---|
| Electrical | `fl_lib/Electrical/Electrical Elements/` (Resistor, Capacitor, Inductor, Electrical Reference), `fl_lib/Electrical/Electrical Sources/` (DC Voltage Source, Controlled Voltage Source, ...), `fl_lib/Electrical/Electrical Sensors/` |
| Mechanical (translational) | `fl_lib/Mechanical/Translational Elements/` (Mass, Translational Spring, Translational Damper, Mechanical Translational Reference) |
| Mechanical (rotational) | `fl_lib/Mechanical/Rotational Elements/` (Inertia, Rotational Spring, Rotational Damper, Mechanical Rotational Reference) |
| Thermal | `fl_lib/Thermal/Thermal Elements/` (Thermal Mass, Conductive Heat Transfer, Thermal Reference) |

Physical ports are named **`LConn1`, `LConn2`, ... (left side)** and **`RConn1`, `RConn2`, ... (right side)**, not `1`, `2`.

## 4. Programmatic model construction

### 4a. Simulink example: PI controller on a first-order plant

```matlab
mdl = 'pi_demo';
if bdIsLoaded(mdl), close_system(mdl, 0); end
new_system(mdl); open_system(mdl);

add_block('simulink/Sources/Step',            [mdl '/Ref'],   'Time','1');
add_block('simulink/Math Operations/Sum',     [mdl '/Err'],   'Inputs','+-');
add_block('simulink/Continuous/PID Controller',[mdl '/PID'],  'P','2','I','1');
add_block('simulink/Continuous/Transfer Fcn', [mdl '/Plant'], ...
          'Numerator','1','Denominator','[1 1]');
add_block('simulink/Sinks/Scope',             [mdl '/Scope']);

add_line(mdl, 'Ref/1',   'Err/1',   'autorouting','on');
add_line(mdl, 'Err/1',   'PID/1',   'autorouting','on');
add_line(mdl, 'PID/1',   'Plant/1', 'autorouting','on');
add_line(mdl, 'Plant/1', 'Err/2',   'autorouting','on');   % feedback
add_line(mdl, 'Plant/1', 'Scope/1', 'autorouting','on');

set_param(mdl, 'StopTime', '10');
Simulink.BlockDiagram.arrangeSystem(mdl);   % tidy layout
out = sim(mdl);
```

### 4b. Simscape example: series RC circuit with scope

```matlab
mdl = 'rc_demo';
if bdIsLoaded(mdl), close_system(mdl, 0); end
new_system(mdl); open_system(mdl);

add_block('nesl_utility/Solver Configuration', [mdl '/Solver']);
add_block('fl_lib/Electrical/Electrical Sources/DC Voltage Source', [mdl '/V']);
add_block('fl_lib/Electrical/Electrical Elements/Resistor',          [mdl '/R']);
add_block('fl_lib/Electrical/Electrical Elements/Capacitor',         [mdl '/C']);
add_block('fl_lib/Electrical/Electrical Elements/Electrical Reference',[mdl '/GND']);
add_block('fl_lib/Electrical/Electrical Sensors/Voltage Sensor',     [mdl '/Vsense']);
add_block('nesl_utility/PS-Simulink Converter',                       [mdl '/PS2S']);
add_block('simulink/Sinks/Scope',                                     [mdl '/Scope']);

set_param([mdl '/V'], 'v0', '5');
set_param([mdl '/R'], 'R',  '1000');  set_param([mdl '/R'], 'R_unit', 'Ohm');
set_param([mdl '/C'], 'c',  '1e-6');  set_param([mdl '/C'], 'c_unit', 'F');

% Physical connections use LConn/RConn port names.
% Port assignments can vary by block/release: confirm with
% get_param(blk,'PortHandles') or the GUI before relying on these.
add_line(mdl, 'V/LConn1', 'R/LConn1');
add_line(mdl, 'R/RConn1', 'C/LConn1');
add_line(mdl, 'C/RConn1', 'GND/LConn1');
add_line(mdl, 'V/RConn1', 'GND/LConn1');
add_line(mdl, 'Solver/RConn1', 'GND/LConn1');
% Sensor across C, then PS-Simulink converter to Scope: wire per the
% sensor's actual port names, then:
add_line(mdl, 'PS2S/1', 'Scope/1');

set_param(mdl, 'StopTime', '0.02');
```

After scripting any Simscape network, **open the model and look at it** (or run `sim`) to catch wrong port wiring. A script that wires the wrong physical ports often still compiles.

### 4c. Scripting conventions

- Signal ports are addressed `'Block/1'`; physical ports `'Block/LConn1'`, `'Block/RConn1'`.
- Pass `'autorouting','on'` to `add_line` for readable diagrams.
- Prefer `set_param` with **string** values, including numbers: `set_param(blk,'Gain','2.5')`.
- Simscape parameters often have a paired unit property: `R` and `R_unit`, `c` and `c_unit`. Set both.
- Use `Simulink.BlockDiagram.arrangeSystem(mdl)` to auto-layout.
- Save with `save_system(mdl, 'name.slx')`. Prefer `.slx` over legacy `.mdl`.

## 5. Running simulations and getting data

```matlab
set_param(mdl, 'StopTime', '10');
out = sim(mdl);                           % returns Simulink.SimulationOutput
```

Programmatic and parametric runs:

```matlab
simIn = Simulink.SimulationInput(mdl);
simIn = simIn.setVariable('Rval', 2200);          % workspace variable override
simIn = simIn.setModelParameter('StopTime','5');
out   = sim(simIn);

% Parameter sweep (parallel if Parallel Computing Toolbox is available)
for k = 1:5
    in(k) = Simulink.SimulationInput(mdl);
    in(k) = in(k).setVariable('Rval', 1000*k);
end
outs = parsim(in);   % or sim(in) for serial
```

Ways to get data out:

| Method | Notes |
|---|---|
| Scope | Fast visual check. Enable "Log data to workspace" in its settings to also save. |
| To Workspace block | Set Save format to `Timeseries`; access as `out.simout`. |
| Signal logging | Right-click a signal → Log Selected Signals; read from `out.logsout`, e.g. `out.logsout.get('name').Values`. |
| Simulation Data Inspector | `Simulink.sdi.view`; best for comparing runs. |
| **Simscape logging** | Turned on per model: `set_param(mdl,'SimscapeLogType','all')`. Read `out.simlog`, or open the Simscape Results Explorer (`sscexplore`). Logs internal physical variables (across/through) without adding sensors. |

## 6. Solver guidance

- **Simscape models** need a stiff-capable solver. Good defaults:
  - Variable-step: `ode15s` or `ode23t`.
  - Fixed-step (needed for code generation / real-time): `ode14x`, `ode3`, or a Simscape **local solver** (enable "Use local solver" in the Solver Configuration block; set a sample time such as 1e-3).
- **Pure Simulink** models: `auto` (default) is usually fine; use `ode45` for non-stiff, `ode15s` for stiff.
- Tight tolerances (`RelTol` 1e-6 and below) rarely fix problems and slow simulation. Fix modeling issues first.
- Small time constants mixed with large ones (stiffness) → use `ode15s`/`ode23t` and set a reasonable **Max step size** to avoid skipping events.
- Switching/PWM circuits: use `ode23t` or local solver with a fixed step smaller than the switching period, and consider Simscape Electrical's averaged or ideal-switch components.

## 7. Debugging checklist

Work through in order:

1. **Update the diagram** (Ctrl+D) and read the *first* error; later errors are often consequences.
2. **"Solver Configuration block missing"** → add one per physical network.
3. **"Network has no reference" / "floating node" / singular matrix** → add the domain reference (e.g., Electrical Reference); ensure every network is grounded.
4. **Algebraic loop (Simulink)** → break with a Unit Delay or Memory block, or restructure; avoid ignoring it unless the loop is intentional.
5. **Domain mismatch ("cannot connect X port to Y port")** → components in different domains need a converter component (e.g., motor, pump, thermal-electrical coupling), not a direct wire.
6. **Initial condition inconsistent / DAE index problems** → loosen initial targets (Variables tab, priority "High" vs "Low"/"None"), or remove stiff ideal elements (e.g., two ideal voltage sources in parallel, rigid mass loops, ideal capacitors in a loop with voltage sources).
7. **Simulation hangs or step size collapses** → inspect for discontinuities (ideal switches, hard stops, friction); add small series resistance/parasitic damping or use smoothed/finite-stiffness elements.
8. **Wrong units** → check converter blocks and "_unit" parameters; confirm the source's magnitude (V vs mV, N vs kN).
9. **Results look flat or zero** → confirm the source is actually enabled/stepping at the right time; check sensor polarity; check that logging is enabled.
10. Use the **Diagnostic Viewer** to click through to the offending block, and **Simscape → Simscape Results Explorer** to see internal states.

## 8. Discovering paths and parameters (use this instead of guessing)

```matlab
% Find a library block by name
find_system('fl_lib', 'Regexp','on', 'Name','Resistor')
find_system('simulink', 'SearchDepth',3, 'Name','Integrator')

% Library path of a block already placed in a model
get_param(gcb, 'ReferenceBlock')

% All editable parameters of a block
get_param(gcb, 'DialogParameters')

% Port structure
ph = get_param(gcb, 'PortHandles');   % fields: Inport, Outport, LConn, RConn

% Open library to browse
open_system('fl_lib'); open_system('simulink');
```

In the GUI: right-click a block → **Help**, or **Library Link → Go to library block**, to find its location and parameter names.

## 9. Simscape language (custom components)

Put the file in a package folder (`+MyLib/resistor.ssc`) or on the MATLAB path, then run `sscprint`-style checks by building the library with `ssc_build('MyLib')`. Minimal example:

```
component my_resistor
    nodes
        p = foundation.electrical.electrical;   % positive terminal
        n = foundation.electrical.electrical;   % negative terminal
    end
    parameters
        R = {1, 'Ohm'};                         % resistance
    end
    variables
        i = {0, 'A'};                           % through variable
        v = {0, 'V'};                           % across variable
    end
    branches
        i : p.i -> n.i;
    end
    equations
        v == p.v - n.v;
        v == i * R;
    end
end
```

Rules of thumb:

- Equations use `==`; assignments (`=`) are only for declarations/parameters.
- Every parameter, variable, and equation should carry **units** (`{1,'Ohm'}`).
- Use `der(x)` for time derivatives; use `if`/`else` sparingly, as discontinuities hurt solver performance.
- Number of equations must equal number of unknowns (variables + internal branch quantities), or the build will report an equation-count mismatch.
- Build the library with `ssc_build` before using it in Simulink; rebuild after edits.

## 10. Best practices

- **Start small.** Build and simulate a minimal network, then add detail incrementally. Re-run after every addition.
- **Name every block and signal** meaningfully; avoid default names like `Gain1`.
- **Use subsystems** for structure; use Subsystem References or Model References for reuse and large projects.
- **Parameterize** with workspace variables or a data dictionary (`.sldd`) rather than hard-coded numbers; keep a setup script (`init_params.m`) that defines them and use a model callback (Model Properties → Callbacks → `PreLoadFcn`/`InitFcn`) to run it.
- **Validate the physics** against a hand calculation (e.g., RC time constant τ = RC; mass-spring natural frequency ω = √(k/m)) before trusting complex models.
- **Keep units consistent**, preferring SI. Simscape handles conversion if you set units explicitly.
- **Use Simscape logging** instead of adding many sensors for diagnostics; use sensors only for signals the controller needs.
- **Version control:** `.slx` is a zipped binary; use Simulink Project and its Git integration, and use `slxmerge`/Model Comparison tools for diffs.
- **Prefer averaged models** for long-horizon power-electronics studies; use switching models only when switching detail matters.
- **Do not over-tighten tolerances or add artificial damping** to hide a structural problem; diagnose the cause instead.

## 11. Responding to user requests (for Claude)

- Ask which MATLAB release and which toolboxes the user has if it affects the answer (block names and libraries shift between releases; Simscape Electrical and Multibody are separate products).
- When providing scripts, **state assumptions** (library paths, port names) and point the user to §8 to verify, since a wrong path or port name is the most common failure.
- Offer both GUI steps and a script when the user's preference is unclear.
- Always mention the **Solver Configuration block and the domain reference** when giving Simscape instructions; these are what users forget.
- Do not claim a model was run or that output values are real unless the user provides them or an execution environment with MATLAB is available. Without MATLAB, give expected-behavior reasoning (time constants, steady-state values) and label it as an estimate.
- For error messages, ask for the **full text of the first error** and the block it points to.
