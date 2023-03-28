# REGIA AUTOMATICA
## Scelte di progettazione
 - Decisione della camera da utilizzare in output in base alle camere top view. Nella decisione non vengono considerate le camere laterali.

## TO-DO
 - Calcolare velocità centroide in ogni camera per avere la quantità di moto
 - Segnali per la chiusura dell'applicazione (SIGINT)
 - gestione eccezioni
 - Sincronizzazione thread (utilizzo di mutex e condition variables al posto di busy wait)
 - Crop output video
 - parsing argomenti: path file di configurazione