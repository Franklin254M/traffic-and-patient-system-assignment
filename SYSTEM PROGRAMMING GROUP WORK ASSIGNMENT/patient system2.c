#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 2
#define MAX_CONDITIONS 3
#define MAX_NAME 50
#define MAX_INFO 100

typedef struct {
    char name[MAX_NAME];
    int is_fatal;
} MedicalCondition;

typedef struct {
    int id, age;
    char name[MAX_NAME], info[MAX_INFO];
    MedicalCondition conditions[MAX_CONDITIONS];
    int condition_count;
} Patient;

Patient* patients = NULL;
int patient_count = 0;
int capacity = INITIAL_CAPACITY;

void clear_input() { while(getchar() != '\n'); }

void add_condition(Patient* p) {
    if (p->condition_count >= MAX_CONDITIONS) {
        printf("Max conditions reached.\n");
        return;
    }
    printf("Condition name (or 'none'): ");
    char name[MAX_NAME];
    fgets(name, MAX_NAME, stdin);
    name[strcspn(name, "\n")] = '\0';
    if (strcasecmp(name, "none") == 0) return;

    printf("Fatal? (y/n): ");
    char fatal;
    scanf(" %c", &fatal);
    clear_input();

    strcpy(p->conditions[p->condition_count].name, name);
    p->conditions[p->condition_count].is_fatal = (tolower(fatal) == 'y');
    p->condition_count++;
}

void add_patient() {
    if (patient_count == capacity) {
        capacity *= 2;
        Patient* temp = realloc(patients, capacity * sizeof(Patient));
        if (!temp) {
            printf("Memory allocation failed!\n");
            return;
        }
        patients = temp;
    }

    Patient* p = &patients[patient_count];
    p->condition_count = 0;

    printf("\nEnter patient ID: ");
    scanf("%d", &p->id);
    clear_input();

    printf("Enter name: ");
    fgets(p->name, MAX_NAME, stdin);
    p->name[strcspn(p->name, "\n")] = '\0';

    printf("Enter age: ");
    scanf("%d", &p->age);
    clear_input();

    while (p->condition_count < MAX_CONDITIONS) {
        add_condition(p);
        if (p->condition_count > 0) {
            printf("Add another? (y/n): ");
            char choice;
            scanf(" %c", &choice);
            clear_input();
            if (tolower(choice) != 'y') break;
        }
    }

    printf("Other info: ");
    fgets(p->info, MAX_INFO, stdin);
    p->info[strcspn(p->info, "\n")] = '\0';

    patient_count++;
    printf("Patient added successfully!\n");
}

void print_patients() {
    if (patient_count == 0) {
        printf("No patients in system.\n");
        return;
    }

    printf("\nPatient Records:\n");
    for (int i = 0; i < patient_count; i++) {
        printf("\nID: %d | Name: %s | Age: %d\n", 
               patients[i].id, patients[i].name, patients[i].age);
        
        printf("Medical Conditions:\n");
        for (int j = 0; j < patients[i].condition_count; j++) {
            printf("  %s%s\n", patients[i].conditions[j].name,
                   patients[i].conditions[j].is_fatal ? " (FATAL)" : "");
        }
        
        if (strlen(patients[i].info) > 0) {
            printf("Additional Info: %s\n", patients[i].info);
        }
    }
}

void update_patient() {
    if (patient_count == 0) {
        printf("No patients to update.\n");
        return;
    }

    printf("Enter patient ID to update: ");
    int id;
    scanf("%d", &id);
    clear_input();

    for (int i = 0; i < patient_count; i++) {
        if (patients[i].id == id) {
            printf("\nUpdating patient %s\n", patients[i].name);
            
            printf("New name (enter to keep): ");
            char name[MAX_NAME];
            fgets(name, MAX_NAME, stdin);
            if (strlen(name) > 1) {
                strcpy(patients[i].name, name);
                patients[i].name[strcspn(patients[i].name, "\n")] = '\0';
            }

            printf("New age (0 to keep): ");
            int age;
            scanf("%d", &age);
            clear_input();
            if (age > 0) patients[i].age = age;

            printf("Update conditions? (y/n): ");
            char choice;
            scanf(" %c", &choice);
            clear_input();
            
            if (tolower(choice) == 'y') {
                patients[i].condition_count = 0;
                while (patients[i].condition_count < MAX_CONDITIONS) {
                    add_condition(&patients[i]);
                    if (patients[i].condition_count > 0) {
                        printf("Add another? (y/n): ");
                        scanf(" %c", &choice);
                        clear_input();
                        if (tolower(choice) != 'y') break;
                    }
                }
            }

            printf("Patient updated successfully!\n");
            return;
        }
    }
    printf("Patient not found.\n");
}

void remove_patient() {
    if (patient_count == 0) {
        printf("No patients to remove.\n");
        return;
    }

    printf("Enter patient ID to remove: ");
    int id;
    scanf("%d", &id);
    clear_input();

    for (int i = 0; i < patient_count; i++) {
        if (patients[i].id == id) {
            for (int j = i; j < patient_count - 1; j++) {
                patients[j] = patients[j + 1];
            }
            patient_count--;

            if (patient_count > 0 && patient_count == capacity / 2) {
                capacity /= 2;
                Patient* temp = realloc(patients, capacity * sizeof(Patient));
                if (temp) {
                    patients = temp;
                    printf("System memory shrunk to %d\n", capacity);
                }
            }

            printf("Patient removed successfully!\n");
            return;
        }
    }
    printf("Patient not found.\n");
}

void cleanup() {
    free(patients);
    patients = NULL;
    patient_count = capacity = 0;
}

int main() {
    patients = malloc(INITIAL_CAPACITY * sizeof(Patient));
    if (!patients) {
        printf("Initial memory allocation failed!\n");
        return 1;
    }

    int choice;
    do {
        printf("\nHospital Patient System\n");
        printf("1. Add Patient\n2. Update Patient\n");
        printf("3. Remove Patient\n4. View Patients\n5. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);
        clear_input();

        switch (choice) {
            case 1: add_patient(); break;
            case 2: update_patient(); break;
            case 3: remove_patient(); break;
            case 4: print_patients(); break;
            case 5: cleanup(); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 5);

    printf("System shutdown.\n");
    return 0;
}