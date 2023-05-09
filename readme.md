# REGIA AUTOMATICA
## Scelte di progettazione
 - Decisione della camera da utilizzare in output in base alle camere top view. Nella decisione non vengono considerate le camere laterali.

## TO-DO
- [x] Resizing prima dell'input a 522x224 per la camera centrale e 576x224 per le camere laterali
- [x] Dare un peso ad ogni camera
- [ ] Il numero di aree in ogni frame può influenzare la scelta della camera da mostrare con parametro nel file di configurazione.
- [ ] Salvare video tresholdato. 
    - [x] Aggiungere la possibilità di visionare l'analisi.
- [x] Migliore gestione del resizing nel displaying.
- [ ] Migliora parsing file di configurazione.
- [ ] Aggiorna UML.
- [ ] Analisi prestazioni: come variano gli fps man mano che si aggiungono sources? Grafici...
    - [x] Aggiugi funzione *fpsToFile* che è possibile attivare e disattivare dal file di configurazione
    - [x] Creazione di grafici.

 ## Output

 Si possono vedere alcuni output intermedi e finali [qui](https://drive.google.com/drive/folders/1LuKnDUDkjfy2jBLMzWWTT03MRcdG15KO?usp=share_link).