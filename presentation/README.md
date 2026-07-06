# Presentation Plan

Plan za kratku prezentaciju od 5-8 minuta.

## 1. Tema I Motivacija

Predstaviti projekat `SREES_2026_Bajramovic_TPFPC` i cilj: konverzija
trofaznih vrijednosti `P+jQ` u polarni oblik.

## 2. Matematicka Osnova

Objasniti formule:

```text
|S| = sqrt(P^2 + Q^2)
angleDeg = atan2(Q, P) * 180 / pi
```

Posebno spomenuti total:

```text
Ptotal, Qtotal, |Stotal|, angleTotal
```

## 3. Balanced Vs Unbalanced Ulaz

Balanced mod koristi unos faze A za sve tri faze. Unbalanced mod dozvoljava
poseban P/Q unos za A, B i C.

## 4. natID GUI

Pokazati glavni prozor aplikacije: input polja, izbor moda, dugme Convert,
rezultate po fazama i total rezultate.

## 5. Dense/Sparse Matrice

Objasniti da tok konverzije prolazi kroz dense input/output matrice i sparse
input/output matrice. Naglasiti da ovo nije Ybus demo.

## 6. Threading I Progress

Spomenuti worker thread za konverziju, progress thread za indikator i siguran
GUI update preko `gui::thread::asyncExecInMainThread(...)`.

## 7. DLL Plugin

Objasniti ordinary DLL plugin kao nacin da core konverzija bude dostupna izvan
glavne GUI aplikacije.

## 8. dTwin Plugin Workflow

Pokazati `Model -> Import -> TPFPC Default Converter`, unos P/Q vrijednosti i
generisanje `.dmodl` modela.

## 9. Test I Validacija

Pokazati balanced i unbalanced test primjere. Spomenuti MATPOWER `t_case3p_a`
kao referentni izvor realnih trofaznih P/Q podataka, bez poredjenja sa cijelim
MATPOWER solverom.

## 10. Zakljucak

Zakljuciti da je projekat cist TPFPC konverter: C++/natID GUI, matrice, plugin-i
i dTwin import tok, bez Newton-Raphson solvera i bez Ybus demo koda.
