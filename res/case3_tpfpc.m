function mpc = case3_tpfpc
%CASE3_TPFPC Small MATPOWER-style demo case for SREES_2026_Bajramovic_TPFPC.
%   Stable parser target: baseMVA, bus, gen and branch matrices.

mpc.version = '2';
mpc.baseMVA = 100;

% bus_i type Pd Qd Gs Bs area Vm Va baseKV zone Vmax Vmin
mpc.bus = [
    1   3   0    0    0   0   1   1.060   0.00    230   1   1.10   0.90;
    2   2   100  30   0   0   1   1.045  -4.98    230   1   1.10   0.90;
    3   1   90   40   0   0   1   1.010  -12.72   230   1   1.10   0.90;
];

% bus Pg Qg Qmax Qmin Vg mBase status Pmax Pmin
mpc.gen = [
    1   232.4  -16.9   10   -10   1.060   100   1   250   10;
    2   40.0    42.4   50   -40   1.045   100   1   300   10;
];

% fbus tbus r x b rateA rateB rateC ratio angle status angmin angmax
mpc.branch = [
    1   2   0.02   0.06   0.030   130   130   130   0   0   1   -360   360;
    1   3   0.08   0.24   0.025   130   130   130   0   0   1   -360   360;
    2   3   0.06   0.18   0.020   65    65    65    0   0   1   -360   360;
];
