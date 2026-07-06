# SREES_2026_Bajramovic_TPFPC

TPFPC = Three Phase Flow Polar Converter.

## Tema

Konvertor trofaznih tokova snaga: polarne koordinate.

## Kratak Opis

Aplikacija je C++/natID GUI alat koji konvertuje trofazne kompleksne snage iz
pravougaonog oblika `P+jQ` u polarni oblik `|S|` i `angleDeg`.

Podrzani su balansirani i nebalansirani ulazi, glavni natID GUI, ordinary DLL
plugin i dTwin import plugin.

## Formule

```text
Sa = Pa + jQa
Sb = Pb + jQb
Sc = Pc + jQc

|S| = sqrt(P^2 + Q^2)
angleDeg = atan2(Q, P) * 180 / pi

Ptotal = Pa + Pb + Pc
Qtotal = Qa + Qb + Qc
|Stotal| = sqrt(Ptotal^2 + Qtotal^2)
angleTotal = atan2(Qtotal, Ptotal) * 180 / pi
```

## Balanced I Unbalanced Rad

- Balanced: korisnik unosi fazu A, a B i C se popunjavaju istim P/Q
  vrijednostima.
- Unbalanced: korisnik rucno unosi P/Q za sve tri faze.

## Tok Podataka

```text
GUI input
-> dense input matrix 3x2
-> sparse input matrix 3x2
-> PolarConverter
-> dense output matrix 3x2
-> sparse output matrix 3x2
-> GUI / dTwin output
```

## Matrice

- Dense input matrix: rows A/B/C, columns P/Q.
- Dense output matrix: rows A/B/C, columns magnitude/angleDeg.
- Sparse input/output matrice cuvaju nenulte ulazne i izlazne vrijednosti.
- Matrice nisu Ybus demo. One su dio stvarnog toka konverzije iz P/Q u polarni
  oblik.

## Threading

- Worker thread radi konverziju.
- Progress thread osvjezava progress indikator.
- GUI update ide preko `gui::thread::asyncExecInMainThread(...)`.

## Plugin-i

- Ordinary DLL plugin: izlozena core konverzija.
- dTwin import plugin: generise `.dmodl` model i pojavljuje se u dTwin pod
  `Model -> Import` kao `TPFPC Default Converter`.

## dTwin Plugin Deployment

Preferirani user plugin folder:

```text
C:\Users\Korisnik\ba.natID\plugins
```

dTwin se pokrece sa privremenim `PATH`-om koji ukljucuje:

```text
C:\Users\Korisnik\ba.natID\plugins
C:\Users\Korisnik\other_bin\bin
C:\Users\Korisnik\other_bin\bin\GTK
```

Ovaj `PATH` se koristi samo za pokretanje dTwin-a i ne mijenja se trajno.

## Sta Nije Dio Projekta

- Newton-Raphson solver nije dio finalnog projekta.
- Ybus demo nije dio finalnog projekta.
- Projekat nije puni load-flow solver.
- MATPOWER se koristi samo kao izvor referentnih P/Q test podataka za validaciju
  konverzije.

## Test Primjeri

Balanced primjer:

```text
A: P = 100, Q = 50
B: P = 100, Q = 50
C: P = 100, Q = 50
```

Unbalanced primjer:

```text
A: P = 100, Q = 50
B: P = 150, Q = 75
C: P = 200, Q = 100
```

MATPOWER referentni P/Q primjer iz `t_case3p_a`:

```text
A: P = 1341.42, Q = 970.52
B: P = 2096.10, Q = 1341.41
C: P = 2672.34, Q = 1894.59
```

Validacija je dokumentovana u:

```text
docs/matpower_validation.md
```

## MATPOWER-to-dTwin polar PF tok

Zbog roka je dodan stabilan end-to-end tok za mali standardni MATPOWER case.
Ovaj tok ne mijenja postojeÃ„â€¡i P/Q trofazni konverter, nego dodaje novi testni
target:

```text
MatpowerToDmodlTest
```

Tok radi:

```text
res/case3_tpfpc.m
-> C++ MATPOWER parser
-> PQ/PV/slack klasifikacija
-> Ybus formiranje
-> polarni Ybus parametri |Y_ij| i theta_ij
-> neto injekcije P_i/Q_i
-> .dmodl model sa NLE jednaÃ„Âinama u polarnim koordinatama
```

PodrÃ…Â¾ani ulaz je stabilan za standardne male MATPOWER case fajlove koji imaju:

- `mpc.baseMVA`
- `mpc.bus`
- `mpc.gen`
- `mpc.branch`

Parser podrÃ…Â¾ava komentare `%`, razmake i zavrÃ…Â¡etak matrice sa `];`. Ovo nije
puni MATLAB interpreter i nije puni MATPOWER parser.

Demo ulaz:

```text
res/case3_tpfpc.m
```

Demo izlaz nakon pokretanja testa:

```text
res/case3_tpfpc_output.dmodl
```

Test ispisuje:

- baseMVA
- broj Ã„Âvorova, generatora i grana
- PQ/PV/slack liste
- broj nenultih elemenata Ybus matrice
- polarne Ybus elemente
- neto injekcije po Ã„Âvoru
- putanju generisanog `.dmodl` fajla


## Glavna funkcionalnost: MATPOWER-to-dTwin polarni power-flow converter

Glavni tok projekta sada je:

```text
MATPOWER .m
-> C++ parser
-> PQ/PV/slack klasifikacija
-> Ybus matrica
-> polarni Ybus elementi |Y_ij| i theta_ij
-> nelinearne power-flow jednaÃ„Âine u polarnim koordinatama
-> dTwin .dmodl model
```

Stari P/Q konverter nije uklonjen. On je ostao kao `P/Q Demo` tab i pokazuje
osnovnu konverziju kompleksne snage iz pravougaonog u polarni oblik.

### GUI MATPOWER mode

U glavnoj aplikaciji postoji tab:

```text
MATPOWER PF Converter
```

U njemu korisnik bira ulazni MATPOWER `.m` fajl i izlazni `.dmodl` fajl, zatim
klikne:

```text
Convert MATPOWER to Polar dTwin Model
```

MATPOWER konverzija se izvrÃ…Â¡ava u worker threadu. Progress indikator se
osvjeÃ…Â¾ava iz posebnog progress threada. GUI thread samo prima klik i prikazuje
rezultat.

GUI prikazuje:

- baseMVA
- broj buses/generators/branches
- broj i listu PQ, PV i slack Ã„Âvorova
- Ybus sparse nnz
- izlaznu `.dmodl` putanju
- success/error status

### DLL MATPOWER API

Ordinary DLL plugin sada ima i MATPOWER funkciju:

```cpp
tpfpcConvertMatpowerToDmodl(inputPath, outputPath, result)
```

Funkcija vraÃ„â€¡a success/failure, error/status poruku, baseMVA, broj
Ã„Âvorova/generatora/grana, broj PQ/PV/slack Ã„Âvorova, Ybus nnz i output path.
PostojeÃ„â€¡a P/Q DLL funkcija nije uklonjena.

### dTwin plugin

dTwin import plugin sada se prikazuje kao:

```text
TPFPC MATPOWER Polar PF Converter
```

Plugin prima MATPOWER `.m` fajl, generiÃ…Â¡e polarni `.dmodl` model i poziva dTwin
completion callback kako bi dTwin mogao uÃ„Âitati generisani model.

### Demo fajlovi

Demo input:

```text
res/case3_tpfpc.m
```

Console/demo output:

```text
res/case3_tpfpc_output.dmodl
```

GUI default output:

```text
res/case3_tpfpc_gui_output.dmodl
```

## MATPOWER three-phase prototype support

Dodata je ogranicena i stabilna podrska za MATPOWER 8 three-phase prototype
format na primjeru `t_case3p_a`.

Podrzana polja su:

- `mpc.baseMVA`
- `mpc.basekVA`
- `mpc.bus3p`
- `mpc.line3p`
- `mpc.xfmr3p` / `mpc.xfrm3p`
- `mpc.shunt3p`
- `mpc.load3p`
- `mpc.gen3p`
- `mpc.lc`

Three-phase parser ne predstavlja puni MATLAB/MATPOWER interpreter. Namjerno je
ogranicen na mali prototip format iz MATPOWER dokumentacije. Fizicki 3p bus se
u internom modelu razvija u fazne cvorove, npr. `1A`, `1B`, `1C`. Iz `line3p`
i `lc` podataka formira se fazna Ybus matrica sa medjufaznim spregama, a zatim
se generise polarni dTwin `.dmodl` model sa varijablama tipa:

```text
V_2A, delta_2A
V_2B, delta_2B
V_2C, delta_2C
```

Novi lokalni demo input:

```text
res/t_case3p_a.m
```

Novi console/demo output:

```text
res/t_case3p_a_output.dmodl
```

Uz three-phase `.dmodl` generise se i companion visual model:

```text
res/t_case3p_a_output.vmodl
```

Visual model sadrzi grafove:

- `Voltage magnitude by phase`: magnitude napona `|V|` po faznim cvorovima
- `Voltage angle by phase`: uglovi napona `delta` po faznim cvorovima, u stepenima

Novi GUI default output:

```text
res/t_case3p_a_gui_output.dmodl
```

Stari standardni MATPOWER case `res/case3_tpfpc.m` i dalje je podrzan.

