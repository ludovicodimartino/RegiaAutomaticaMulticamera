# REGIA AUTOMATICA
Il programma è in grado di effettuare la regia automatica di *n* camere date in input attraverso il [file di configurazione](./scene.conf). 
Il risultato della regia è salvato dove definito dal parametro *outPath* sotto l'etichetta [OUT].

Il programma necessita delle librerie di OpenCV per funzionare. Le istruzioni per il setup di OpenCV si trovano [qui](./workspace%20configuration/VScode_configurations_c%2B%2B_and_OpenCV.md).

## Struttura del programma

Il programma si compone di due classi:

![](./diagrams/macrostruttura.svg)

La classe [*Capture*](./src/capture.h) si occupa di gestire ogni camera a partire dall’apertura del flusso video
fino alla lettura e al calcolo del punteggio di ogni singolo frame. In questo modulo sono definiti dei
metodi per il calcolo dello score considerando criteri come l’area occupata dai giocatori nel frame, la
velocità e il numero di giocatori. È possibile scrivere nuovi metodi che considerino altri parametri e
utilizzino una diversa logica per il calcolo del punteggio dei frame. Per ora i metodi definiti sono *FrameDiffAreaAndVel()* e *FrameDiffAreaOnly()*. Prendendo ispirazione da questi si possono [creare nuovi metodi](#creazione-di-un-nuovo-metodo-per-il-calcolo-dello-score).

La componente [*Scene*](./src/scene.h), al contrario di Capture, è di più alto livello. Infatti, si occupa di interfacciarsi
con Capture per recuperare i punteggi delle diverse camere grazie ai quali scegliere, in ogni momento,
il frame da mostrare in uscita. Inoltre, si occupa dell’avvio e della terminazione del programma e
dei threads. Difatti, l’analisi di ogni camera in Capture è eseguita in un thread a parte.

![](./diagrams/thread.svg)

Una migliore comprensione della struttura del software la fornisce il [diagramma delle calssi UML](./diagrams/uml.pdf).

## Creazione di un nuovo metodo per il calcolo dello score

Per creare un nuovo metodo è necessario seguire i seguenti passi:

 1. Creazione dell'intestazione del metodo nel file [Capture.h](./src/capture.h) sotto la sezione *public*. Esempio:
    
        void randomScore();

2. Implementazione della funzione in [Capture.cpp](./src/capture.cpp). Esempio:

        void Capture::randomScore(){
                printf("randomScore thread ID: %d Name: %s\n",std::this_thread::get_id(), capName.c_str());
                cv::Mat originalFrame;
                while(isOpened()){
                    active = true;
                    if(!read(originalFrame))break;
                    
                    // Check whether a stop signal has been received
                    if(stopSignalReceived){
                        readyToRetrieve = true;
                        break;
                    }
                  
                    //acquire lock
                    std::unique_lock lk(mx);
                    condVar.wait(lk, [this] {return !readyToRetrieve;});
                    
                    score = rand()%5001; // update the score

                    frame.release();
                    frame = originalFrame.clone(); // update the frame
                    readyToRetrieve = true;
                    
                    // Unlock and notify
                    lk.unlock();
                    condVar.notify_one();
                    
                    originalFrame.release();
                    ++processedFrameNum;
                }
                active = false;
                condVar.notify_one();
        }

3. Aggiungere l'etichetta del metodo appena creato in [*Scene.cpp*](./src/scene.cpp). Esempio:

        //Init method labels
        methodLabels.insert({{"FrameDiffAreaAndVel", &Capture::FrameDiffAreaAndVel},
                            {"FrameDiffAreaOnly", &Capture::FrameDiffAreaOnly}
                            {"Random", &Capture::randomScore}});

4. Impostare il parametro *method* nel [file di configurazione](./scene.conf) con il valore scritto all'interno dell'etichetta. Esempio:

        method=Random

5. Testare il corretto funzionamento del programma.

 ## Output

 Si possono vedere alcuni output intermedi e finali [qui](https://drive.google.com/drive/folders/1LuKnDUDkjfy2jBLMzWWTT03MRcdG15KO?usp=share_link).