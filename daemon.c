#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<syslog.h>
#include<fcntl.h>
#include<time.h>
#include<sys/wait.h>
#include<termios.h>
#include<signal.h>

int keepRunning = 1;
int start = 0;

// Daemon beenden wenn keepRunning Variable auf 0 gesetzt wurde
void handleSignal(int signal) {
    if (signal == SIGTERM) {
        keepRunning = 0;
	    
    }
	exit(0);
} 

void ping() {
    printf("Welche Seite möchtest du pingen? Gib den Link ein: \n");
    char link[100]; // Annahme: Link hat maximal 100 Zeichen
    scanf("%99s", link); // Einlesen des Links von der Tastatur (maximal 99 Zeichen, um Platz für das Nullzeichen zu lassen)

    if (execl("/bin/ping", "ping", "-c", "5", link, NULL) == -1) {
        fprintf(stderr, "Fehler beim Ausführen von ping\n");
        exit(1);
    }

    // Zurück zur Menüfunktion nach Abschluss von ping()
}


void Uhrzeit() {
    while (1) {
        // Erzeugung eines Kindprozesses
        pid_t pid = fork();

        if (pid == -1) {
            perror("Fehler bei fork");
            exit(1);
        } else if (pid == 0) {
            // Kindprozess: Verkettung mit dem "date"-Programm
            if (execl("/bin/date", "date", NULL) == -1) {
                perror("Fehler beim Ausführen von date");
                exit(1);
            }
        } else {
            // Elternprozess: Tastatureingabe erkennen
            struct termios old, new;
            tcgetattr(STDIN_FILENO, &old);
            new = old;
            new.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &new);
            fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

            // Wartezeit zwischen den Befehlsausführungen (1 Sekunde)
            sleep(1);

            // Prüfen, ob eine Taste gedrückt wurde
            char ch;
            if (read(STDIN_FILENO, &ch, 1) == 1) {
                // Programm beenden
                tcsetattr(STDIN_FILENO, TCSANOW, &old);
                fcntl(STDIN_FILENO, F_SETFL, 0);
                break;
            }

            // Warten auf den Abschluss des Kindprozesses
            wait(NULL);
        }
    }
}

void printProcessInfo() {
    // Prozess ID erhalten
    pid_t pid = getpid();
    printf("Elternprozess ID: %d\n", pid);

    // Rechte erhalten
    mode_t mode = umask(0);
    umask(mode);
    printf("Rechte: %o\n", mode);

    // Benutzer-ID erhalten
    uid_t uid = getuid();
    printf("Benutzer-ID: %d\n", uid);

    // Gruppen-ID erhalten
    gid_t gid = getgid();
    printf("Gruppen-ID: %d\n", gid);

    // RAM-Ausnutzung erhalten
    char statm_path[64];
    sprintf(statm_path, "/proc/%d/statm", pid);

    int statm_fd = open(statm_path, O_RDONLY);
    if (statm_fd != -1) {
        char buffer[256];
        ssize_t bytesRead = read(statm_fd, buffer, sizeof(buffer) - 1);
        close(statm_fd);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';

            unsigned long size, resident, share, text, lib, data, dt;
            sscanf(buffer, "%lu %lu %lu %lu %lu %lu %lu", &size, &resident, &share, &text, &lib, &data, &dt);

            printf("RAM-Ausnutzung:\n");
            printf("Gesamtgröße Ram ausnutzung: %.2f MB\n", (float)size / 1024);

            printf("Speicherunterteilung: \n");
            printf("Resident: %.2f MB\n", (float)resident / 1024);
            printf("Shared: %.2f MB\n", (float)share / 1024);
            printf("Text: %.2f MB\n", (float)text / 1024);
            printf("Library: %.2f MB\n", (float)lib / 1024);
            printf("Data + Stack: %.2f MB\n", (float)data / 1024);
            printf("Dirty Pages: %.2f MB\n", (float)dt / 1024);
        }
    } else {
        printf("Fehler beim Lesen der RAM-Ausnutzung.\n");
    }

    // CPU-Zeit erhalten
    clock_t cpu_time = clock();
    double cpu_seconds = (double)cpu_time / CLOCKS_PER_SEC;
    printf("CPU-Zeit: %.2f Sekunden\n", cpu_seconds);
}

void writeinFile() {
    // Dateiname angeben
    const char* filename = "Messdaten.txt";
    
    // Datei im Schreibmodus öffnen
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Fehler beim Öffnen der Datei. \n");
        exit(1);
    }
    
    // Informationen in die Datei schreiben
    fprintf(file, "Elternprozess ID: %d\n", getpid());
    fprintf(file, "Benutzer ID: %d\n", getuid());
    fprintf(file, "Gruppen ID: %d\n", getegid());
    
    
    // Datei schließen
    fclose(file);
    
    printf("Datei erstellt und Informationen in folgender Datei geschrieben: %s\n", filename);
    printf(" ");
}

     void readResultsFromFile(){
	//Dateiname angeben
         const char* filename = "Messdaten.txt";
	// Datei im Lesemodus öffnen
    FILE* file = fopen(filename, "r");
    if(file == NULL){
        printf("Fehler beim Öffnen der Datei. \n");
        exit(1);
    }
    	// Zeichenweise aus der Datei lesen und in der Konsole anzeigen
	int c;
    while ((c = fgetc(file)) != EOF)
    {
        putchar(c);
    }
	// Datei schließen
    fclose(file);
    }


void printPermissions(mode_t mode) {
    //Abfrage ob wir eine Brechtigung haben oder nicht, wenn ja wird der Linke Teil geprintet bei Nein der Rechte
    printf("Permissions: ");
    //Leseberechtigung (user)
    printf((mode & S_IRUSR) ? "r" : "-");
    //Schreibeberechtigung (user)
    printf((mode & S_IWUSR) ? "w" : "-");
    //Ausführberechtigung (user)
    printf((mode & S_IXUSR) ? "x" : "-");
    //Leseberechtigung (group)
    printf((mode & S_IRGRP) ? "r" : "-");
    //Schreibeberechtigung (group)
    printf((mode & S_IWGRP) ? "w" : "-");
    //Ausführberechtigung (group)
    printf((mode & S_IXGRP) ? "x" : "-");
    //Leseberechtigung (others)
    printf((mode & S_IROTH) ? "r" : "-");
    //Schreibberechtigung (others)
    printf((mode & S_IWOTH) ? "w" : "-");
    //Ausführberechtigung (others)
    printf((mode & S_IXOTH) ? "x" : "-");
    printf("\n");
}

void showCPUTime() {
    clock_t start_t, end_t;
   double total_t;
   int i;
    // Die Funktion Clock wird aufgerufen
   
   start_t = clock();



   end_t = clock();
    // Rechnung um auf Sekunden zu gelangen 
   total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
   printf("Total time taken by CPU: %f\n", total_t  );


}


    void daemonize() {

    // Öffnen des Syslog-Kanals
    openlog("BSRN_ALT4.c", LOG_PID | LOG_CONS, LOG_USER); 

    syslog(LOG_INFO, "Log message from user %d", getuid());

    // Einen Kindprozess erzeugen
    int pid_d = fork();
    syslog(LOG_INFO, "ein Kindprozess wurde erzeugt %d", getpid());
    
    // Es kam beim fork zu einem Fehler
    if (pid_d < 0) {
        perror("Es kam bei fork zu einem Fehler!\n");
        syslog(LOG_ERR, "Es trat ein fork Fehler auf");
        // Programmabbruch
        exit(1);
    }
    
    // Elternprozess
    if (pid_d > 0) {    
        printf("Elternprozess: PID: %i\n", getpid());
        syslog(LOG_INFO, "Elternprozess %d", getppid());
        
    }
    
    // Kindprozess
    if (pid_d == 0) {
        printf("Kindprozess erstellt mit der PID: %i\n", getpid());
        syslog(LOG_INFO,  "befinden uns im Kindprozess");
        
        // Starte eine neue Sitzung mit setsid()
        if (setsid() < 0) {
            fprintf(stderr, "Fehler beim Starten einer neuen Sitzung\n");
            syslog(LOG_ERR, "Fehler beim starten einer neuer Sitzung");
            exit(1);
        }
        
        
        printf("Neue Sitzung gestartet!\n");
        printf("Sitzungsführer: %d\n", getpgrp());
        syslog(LOG_INFO, "Gruppenführer  %d", getpgrp());
    
        
        // Verkettung des Kindprozesses mit einem anderen Programm

        
        printf("Welches Programm möchtest du mit dieser neuen Session verketten?\n");
        printf("\n");
        printf("1. Ping\n");
        printf("2. Uhrzeit\n");
        printf("3. ---Zurück---\n");

        int programmAuswahl;
        scanf("%d", &programmAuswahl); // Einlesen der Wahl von der Tastatur

    
        switch (programmAuswahl) {
            case 1:
                ping();
                break;
            case 2:
                Uhrzeit();
                break;
            case 3:
                Menüanzeigen();
                break;
            default:
                printf("Ungültige Wahl.\n");
                break;
        }

    
    }
     //schließen vom Syslog Kanal
    closelog();
}
void zurückZumMenü() {
    char antwort;

     //   pufferLeeren();

    do {
        printf("Willst du zurück zum Menü? (j/n): ");
        scanf(" %c", &antwort);
        
        if (antwort == 'j' || antwort == 'J') {
            

            Menüanzeigen();
            break;
        } else if (antwort == 'n' || antwort == 'N') {
            exit(1);

            
            break;
        } else {
            printf("Falsche Eingabe. Bitte gib 'j' oder 'n' ein.\n");
        }
    } while (1);
}

void Menüanzeigen(){

        printf(" ██████████                                                         \n");
        printf("░░███░░░░███                                                        \n");
        printf(" ░███   ░░███  ██████    ██████  █████████████    ██████  ████████  \n");
        printf(" ░███    ░███ ░░░░░███  ███░░███░░███░░███░░███  ███░░███░░███░░███ \n");
        printf(" ░███    ░███  ███████ ░███████  ░███ ░███ ░███ ░███ ░███ ░███ ░███ \n");
        printf(" ░███    ███  ███░░███ ░███░░░   ░███ ░███ ░███ ░███ ░███ ░███ ░███ \n");
        printf(" ██████████  ░░████████░░██████  █████░███ █████░░██████  ████ █████\n");
        printf("░░░░░░░░░░    ░░░░░░░░  ░░░░░░  ░░░░░ ░░░ ░░░░░  ░░░░░░  ░░░░ ░░░░░ \n");

        
        printf("----------------------------------------------------------------\n");
        printf("Du bist nun im Menü wähle eine Zahl von 1 - 4 \n");
        printf("1. Daemon starten\n");
        printf("2. Prozessinformationen anzeigen und in Datei speichern\n");
        printf("3. Prozessinformationen aus der gespeicherten Datei auslesen\n");
        printf("4. Menü verlassen und Daemon beenden\n");
        printf("-----------------------------------------------------------------Eingabe: ");


    int eingabe;
    scanf("%d", &eingabe);

        
        switch (eingabe){
            case 1:  
            daemonize();
            break;

            case 2:
            writeinFile();
            printProcessInfo();

            // geben die Datei an, aus der wir die Berechtigung wollen (Der Pfad muss für andere Benutzer geändert werden!)
         char filename[] = "/home/florian/Documents/VS/BSRN_alt4.c"; 
         struct stat fileStat;
        //Fehler anzeugen falls man nicht an die Information gelangt ist
        if (stat(filename, &fileStat) < 0) {
        printf("Error: Unable to retrieve file information.\n");
        }

        printf("Program: %s\n", filename);
        printPermissions(fileStat.st_mode);
        
            showCPUTime();
            zurückZumMenü();
            break;

            case 3:
            readResultsFromFile();
            zurückZumMenü();
            break;

            case 4:
            keepRunning = 0;
            break;
    
            default:
            printf("Ungültige Eingabe. Bitte wähle erneut \n");
            break;
        }
    }


/*
// Puffer leeren, die Tastatur eingabe leeren

void pufferLeeren(){
    int c;
while ((c = getchar()) != '\n' && c != EOF);

}
*/


int main() {
    
    signal(SIGTERM, handleSignal);

    Menüanzeigen();

}   
