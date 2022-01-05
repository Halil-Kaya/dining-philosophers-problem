#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct Philosopher{
    int _id;
    int _tableId;
    int state; //0-> waiting, 1-> eating, 2-> thinking, 3-> finish
    float thinkingTime;
    float eatingTime;
    int eatenRiceAmount;
    pthread_t diningThread;
    pthread_mutex_t lock;
} Philosopher;

typedef struct Table{
    int _id;
    int fullChairCount;
    int emptyChairCount;
    int finishedCount;
    int isOpened;
    int riceAmount;
    int eatenRiceAmount;
    int reOrderAmount;
    int inWhichChair;
    float invoice;
    Philosopher **philosophersOfTable;
    pthread_mutex_t lock_full;
    pthread_mutex_t lock_finish;
    pthread_mutex_t lock_order;
} Table;

Philosopher *philosophers;
Table *tables;

Philosopher* getCurrentPhilosopher();
Table* getTableById(int);
void* initialPhilosopherThreads(void* arg);
void runPhilosopherLoop();
void initialize(int tableCount,int philosopherCountOfTable,int chairCount,int totalPhilosopherCount);
void initialTable(int tableCount,int philosopherCountOfTable,int chairCount);
void initialPhilosopher(int totalPhilosopherCount,int tableCount);
void lockTable(Table *table);
void began(int tableCount,int philosopherCountOfTable,int totalPhilosopherCount);
void startPhilosopherThreads(int totalPhilosopherCount);

double masaAcmaUcreti = 99.90;
double masaTazelemeUcreti = 19.90;
double kiloBasiPrincUcreti = 20;
double totalInvoice = 0;

int main(){
    initialize(8, 8, 8, 80);
    began(8,8,80);
    printf("toplam fiyat -> %d\n",totalInvoice);
    return 0;
}

void initialize(int tableCount,int philosopherCountOfTable,int chairCount,int totalPhilosopherCount){
    initialTable(tableCount,philosopherCountOfTable,chairCount);
    initialPhilosopher(totalPhilosopherCount,tableCount);
}

void began(int tableCount,int philosopherCountOfTable,int totalPhilosopherCount){
    startPhilosopherThreads(totalPhilosopherCount);
}   

void startPhilosopherThreads(int totalPhilosopherCount){
    for(int i = 0; i < totalPhilosopherCount; i++){
        pthread_create(&philosophers[i].diningThread, NULL, &initialPhilosopherThreads, NULL);
    }
    for(int i = 0; i < totalPhilosopherCount; i++){
        printf("%d id li filozofun threadi basladi\n",philosophers[i]._id);
        pthread_join(philosophers[i].diningThread, NULL);
    }
}

Philosopher* getCurrentPhilosopher(){
    for(int i = 0; i < 80; i++){
        if(pthread_equal(pthread_self(),philosophers[i].diningThread)) {
            return &philosophers[i];
        }
    }
    return NULL;
}

Table* getTableById(int tableId){
    for(int i = 0; i < 8; i++){
        if(tables[i]._id == tableId) return &tables[i];
    }
    return NULL;
}

void* initialPhilosopherThreads(void* arg){
    for(int i = 0; i < 8; i++){
        if(tables[i].emptyChairCount > 0){
            Philosopher *philosopher;
            philosopher = getCurrentPhilosopher();
            philosopher->_tableId = i;
            philosopher->state = 0;
            tables[i].philosophersOfTable[tables[i].inWhichChair] = philosopher;
            tables[i].inWhichChair++;
            tables[i].emptyChairCount--;
            printf("! -> %d id li filozof %d id li masaya oturdu \n",philosopher->_id,tables[i]._id);
            pthread_mutex_lock(&tables[i].lock_full);
            if(tables[i].isOpened == 0 && tables[i].emptyChairCount == 0){
                tables[i].isOpened = 1;
                tables[i].riceAmount = 2000;
                tables[i].invoice = masaAcmaUcreti + 40;
                for(int j = 0; j < 8; j++){
                    tables[i].philosophersOfTable[j]->state = 1; // eating
                }
                printf("!! -> %d id li masa acildi \n",tables[i]._id);
            } 
            pthread_mutex_unlock(&tables[i].lock_full);
            runPhilosopherLoop();
            break;
        }
    }
    pthread_exit(NULL);
}

void runPhilosopherLoop(){
    Philosopher *philosopher = getCurrentPhilosopher();
    while(1){
        Table *table = getTableById(philosopher->_tableId);
        if(table->eatenRiceAmount >= 2000){
            pthread_mutex_lock(&table->lock_full);
            int isDone = 1;
            for(int i = 0; i < 8; i++){
                if(table->philosophersOfTable[i]->eatenRiceAmount <= 0) isDone = 0;
            }
            if(isDone){
                printf("<---Fatura %d--->\n", table->_id);
                double totalEatenRice = 0;
                for(int i = 0; i < 8; i++){
                    printf("id -> %d  yenilen miktar -> %d>\n", table->philosophersOfTable[i]->_id, table->philosophersOfTable[i]->eatenRiceAmount);
                    totalEatenRice += table->philosophersOfTable[i]->eatenRiceAmount;
                }
                printf("id -> %d masasinda filozoflarin toplam yedigi pirinc miktari -> %d",table->_id,totalEatenRice);
                printf("id -> %d masasinda toplam yenilen pirinc miktari : -> %d\n", totalEatenRice * table->reOrderAmount);
                printf("id -> %d masasi kac defa tekrar acildi: -> %d\n", table->reOrderAmount);
                printf("id -> %d masasinin odemesi gereken fiyati -> %f\n", table->invoice);
                totalInvoice += table->invoice;
                for(int i = 0; i < 8; i++){
                    table->philosophersOfTable[i]->state = 3;
                }
                table->isOpened = 0;
                table->invoice = 0;
                table->eatenRiceAmount = 0;
                table->finishedCount = 0;
                table->reOrderAmount = 0;
                table->emptyChairCount = 8;
                pthread_mutex_unlock(&table->lock_full);
            }else{
                table->reOrderAmount += 1; 
                table->riceAmount = 2000;
                table->invoice += masaTazelemeUcreti;
                pthread_mutex_unlock(&table->lock_full);
            }
        }else{
            if(philosopher->state == 0){
                sleep(0.2);
            }else if(philosopher->state == 1){            
                Table *table = getTableById(philosopher->_tableId);
                philosopher->eatenRiceAmount += 100;
                table->riceAmount -= 100;
                table->eatenRiceAmount += 100;
                philosopher->state = 2;
                printf("--time--> %d\n",philosopher->eatingTime);
                sleep(philosopher->eatingTime);
            }else if(philosopher->state == 2){
                philosopher->state = 1;
                sleep(philosopher->thinkingTime);
            }else if(philosopher->state == 3){
                pthread_exit(NULL);
            }
        }
    }
}

void initialPhilosopher(int totalPhilosopherCount,int tableCount){
    philosophers = (Philosopher*) calloc(totalPhilosopherCount, sizeof(Philosopher));
    for(int i = 0; i < totalPhilosopherCount; i++){
        pthread_t ptid;
        Philosopher philosopher;
        philosopher._id = i;
        philosopher.diningThread = ptid;
        philosopher.eatenRiceAmount = 0;
        philosopher._tableId = -1;
        philosopher.thinkingTime = ((float)rand()/(float)(RAND_MAX/5)) / 10;
        philosopher.eatingTime = ((float)rand()/(float)(RAND_MAX/5)) / 10;
        philosophers[i] = philosopher;
        printf("%d id li filozof olusturuldu\n",philosopher._id);
    }
}

void initialTable(int tableCount,int philosopherCountOfTable,int chairCount){
    tables = (Table*) calloc(tableCount, sizeof(Table));
    for(int i = 0; i < tableCount; i++){
        Table newTable;
        newTable._id = i;
        newTable.isOpened = 0;
        newTable.eatenRiceAmount = 0;
        newTable.inWhichChair = 0;
        newTable.philosophersOfTable = (Philosopher**)malloc(philosopherCountOfTable * sizeof(Philosopher*));
        pthread_mutex_init(&newTable.lock_full, NULL);
        newTable.fullChairCount = chairCount;
        newTable.emptyChairCount = chairCount;
        tables[i] = newTable;
        printf("%d id li masa olusturuldu\n",newTable._id);
    }
}

void lockTable(Table *table){
    pthread_mutex_lock(&table->lock_full);
    if(table->isOpened == 0 && table->emptyChairCount == 0){
        table->isOpened = 1;
        table->riceAmount = 2000;
        for(int i = 0; i < 8; i++){
            table->philosophersOfTable[i]->state = 1;
        }
    }
    pthread_mutex_unlock(&table->lock_full);
}
