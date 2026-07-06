# MATPOWER Reference Validation

This validation uses MATPOWER only as a reference source of realistic
three-phase P/Q data. TPFPC does not parse MATPOWER files and does not compare
against the full MATPOWER power-flow solver.

The validated scope is:

```text
MATPOWER P/Q per phase -> TPFPC P+jQ to polar conversion
```

## Source Case

Reference case:

- MATPOWER three-phase how-to: https://matpower.org/documentation/howto/three-phase.html
- MATPOWER GitHub test cases: https://github.com/MATPOWER/matpower/tree/master/lib/t

The MATPOWER documentation runs `t_case3p_a` with the `mp.xt_3p` extension and
shows the three-phase generator results used here:

```text
Phase A: P = 1341.42, Q = 970.52
Phase B: P = 2096.10, Q = 1341.41
Phase C: P = 2672.34, Q = 1894.59
```

## Formula

For each phase:

```text
mag = sqrt(P^2 + Q^2)
angleDeg = atan2(Q, P) * 180 / pi
```

For totals:

```text
Ptotal = Pa + Pb + Pc
Qtotal = Qa + Qb + Qc
magTotal = sqrt(Ptotal^2 + Qtotal^2)
angleTotalDeg = atan2(Qtotal, Ptotal) * 180 / pi
```

## Expected Results

```text
Phase A: mag = 1655.6922077488, angleDeg = 35.8858120250
Phase B: mag = 2488.5771031053, angleDeg = 32.6174202191
Phase C: mag = 3275.8010232155, angleDeg = 35.3352273780

Total P = 6109.86
Total Q = 4206.52
Total mag = 7417.8972579836
Total angleDeg = 34.5466566573
```

All six MATPOWER P/Q values are non-zero, so the expected sparse matrix counts
are:

```text
sparse input nnz = 6
sparse output nnz = 6
```

## Project Test

The project contains a dedicated executable test:

```text
MatpowerReferenceTest
```

The test checks:

- dense input matrix values for A/B/C P/Q
- dense polar output matrix magnitude and angleDeg values
- sparse input non-zero count
- sparse output non-zero count
- total P/Q/magnitude/angle values

Run it from the Release output folder after build:

```text
C:\Users\Korisnik\natID.RAMDisk\Out\SREES_2026_Bajramovic_TPFPC\Release\MatpowerReferenceTest.exe
```

Expected output:

```text
MATPOWER t_case3p_a reference conversion: OK
```
