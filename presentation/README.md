Plan prezentacije

1. Uvod i tema projekta
   Predstaviti naziv projekta SREES_2026_Bajramovic_TPFPC i temu: konvertor trofaznih tokova snaga u polarne koordinate. Ukratko objasniti da je projekat realizovan u C++ uz natID GUI, DLL plugin i dTwin integraciju.

2. Motivacija i cilj
   Objasniti zašto je korisno analizirati trofazne elektroenergetske sisteme po fazama A, B i C. Naglasiti da polarni oblik omogućava pregled modula i faznog ugla, što je prirodan zapis za napone, admitanse i jednačine tokova snaga.

3. Glavna funkcionalnost projekta
   Prikazati tok podataka: MATPOWER three-phase .m fajl -> parser -> fazni čvorovi A/B/C -> fazni Ybus -> polarni zapis |Y| i theta -> nelinearne jednačine tokova snaga -> dTwin .dmodl model. Naglasiti da je glavna funkcionalnost projekta MATPOWER-to-dTwin konverzija, dok je P/Q demo pomoćni dio.

4. MATPOWER ulazni podaci
   Objasniti da se MATPOWER koristi kao izvor strukturisanih three-phase podataka, a ne kao solver. Navesti glavne ulazne sekcije koje parser koristi, kao što su baseMVA, bus3p, line3p, load3p, gen3p i xfmr3p.

5. Fazni čvorovi i Ybus matrica
   Objasniti da se svaki fizički bus razvija u fazne čvorove, npr. bus 2 -> 2A, 2B, 2C. Zatim objasniti da se formira fazni Ybus koji uključuje veze između faza i čuva se kao sparse matrica.

6. Polarne jednačine tokova snaga
   Predstaviti osnovne jednačine za P_i i Q_i u polarnom obliku. Objasniti razliku između PQ, PV i slack čvorova: PQ ima P i Q jednačinu, PV ima P jednačinu i zadani napon, a slack čvor predstavlja referentni napon i ugao.

7. natID GUI i threading
   Objasniti da GUI omogućava izbor MATPOWER .m fajla, izbor izlaznog .dmodl fajla, pokretanje konverzije i prikaz statusa. Naglasiti da se konverzija izvršava u worker thread-u, progress indikator u posebnom thread-u, a GUI thread prikazuje rezultat korisniku.

8. DLL plugin i dTwin integracija
   Objasniti ordinary DLL plugin kao odvojeni sloj koji izlaže funkciju konverzije. Zatim objasniti dTwin import plugin koji se pojavljuje u Model -> Import i omogućava generisanje .dmodl modela direktno iz dTwin-a.

9. Testiranje i validacija
   Prikazati testiranje na MATPOWER three-phase slučaju t_case3p_a. Navesti rezultate: 12 faznih čvorova, 9 PQ čvorova, 0 PV čvorova, 3 slack čvora i Ybus sparse nnz = 78. Naglasiti da se generišu .dmodl i .vmodl fajlovi.

10. Zaključak
    Sumirati da projekat ispunjava zahtjeve: C++ implementacija, GUI, konverzija u posebnom thread-u, real-time progress indikator u drugom thread-u, DLL plugin i dTwin integracija. Zaključiti da projekat omogućava automatsku konverziju MATPOWER three-phase ulaza u dTwin model u polarnim koordinatama.

Procijenjeno vrijeme prezentovanja: 7 minuta.