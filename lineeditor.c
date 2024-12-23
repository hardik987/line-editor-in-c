#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 25
#define MAX_LINE_LENGTH 1024

typedef struct {
    int lineNo;
    int position;
} Cursor;

typedef struct {
    char **lines;
    int lineCount;
    Cursor cursor;
    char **undoBuffer[3];
    int undoLineCount[3];
    int undoIndex;
    char **redoBuffer[3];
    int redoLineCount[3];
    int redoIndex;
} Buffer;

// Function to initialize buffer
Buffer* initBuffer() {
    Buffer* buf = (Buffer*)malloc(sizeof(Buffer));
    buf->lines = (char**)malloc(MAX_LINES * sizeof(char*));
    buf->lineCount = 0;
    buf->cursor.lineNo = -1;
    buf->cursor.position = -1;
    buf->undoIndex = 0;
    buf->redoIndex = 0;
    return buf;
}

// Function to save current state for undo
void saveForUndo(Buffer* buf) {
    if (buf->undoIndex >= 3) {
        // Shift undo buffer
        for (int i = 0; i < 2; i++) {
            buf->undoBuffer[i] = buf->undoBuffer[i + 1];
            buf->undoLineCount[i] = buf->undoLineCount[i + 1];
        }
        buf->undoIndex = 2;
    }

    buf->undoBuffer[buf->undoIndex] = (char**)malloc(buf->lineCount * sizeof(char*));
    buf->undoLineCount[buf->undoIndex] = buf->lineCount;

    for (int i = 0; i < buf->lineCount; i++) {
        buf->undoBuffer[buf->undoIndex][i] = strdup(buf->lines[i]);
    }
    buf->undoIndex++;
}

// Function to read file into buffer
void readFile(Buffer* buf, FILE* fp) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, fp) && buf->lineCount < MAX_LINES) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;
        buf->lines[buf->lineCount] = strdup(line);
        buf->lineCount++;
    }
}

// Function to write buffer to file
void writeFile(Buffer* buf, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error opening file for writing\n");
        return;
    }

    for (int i = 0; i < buf->lineCount; i++) {
        fprintf(fp, "%s\n", buf->lines[i]);
    }
    fclose(fp);
}

// Search function
Cursor search(Buffer* buf, const char* word) {
    Cursor cursor = {-1, -1};
    for (int i = 0; i < buf->lineCount; i++) {
        char* found = strstr(buf->lines[i], word);
        if (found) {
            cursor.lineNo = i;
            cursor.position = found - buf->lines[i];
            break;
        }
    }
    return cursor;
}

// Insert line at cursor position
void insertLine(Buffer* buf, const char* line) {
    if (buf->lineCount >= MAX_LINES) {
        printf("Buffer full\n");
        return;
    }

    saveForUndo(buf);

    if (buf->cursor.lineNo == -1) {
        buf->cursor.lineNo = buf->lineCount;
    }

    // Shift lines down
    for (int i = buf->lineCount; i > buf->cursor.lineNo; i--) {
        buf->lines[i] = buf->lines[i - 1];
    }

    buf->lines[buf->cursor.lineNo] = strdup(line);
    buf->lineCount++;
}

// Delete line at cursor position
void deleteLine(Buffer* buf) {
    if (buf->cursor.lineNo < 0 || buf->cursor.lineNo >= buf->lineCount) {
        printf("Invalid cursor position\n");
        return;
    }

    saveForUndo(buf);

    free(buf->lines[buf->cursor.lineNo]);
    
    // Shift lines up
    for (int i = buf->cursor.lineNo; i < buf->lineCount - 1; i++) {
        buf->lines[i] = buf->lines[i + 1];
    }
    
    buf->lineCount--;
}

// Update line at cursor position
void updateLine(Buffer* buf, const char* newText) {
    if (buf->cursor.lineNo < 0 || buf->cursor.lineNo >= buf->lineCount) {
        printf("Invalid cursor position\n");
        return;
    }

    saveForUndo(buf);
    free(buf->lines[buf->cursor.lineNo]);
    buf->lines[buf->cursor.lineNo] = strdup(newText);
}

// Undo function
void undo(Buffer* buf) {
    if (buf->undoIndex == 0) {
        printf("Nothing to undo\n");
        return;
    }

    // Save current state for redo
    if (buf->redoIndex < 3) {
        buf->redoBuffer[buf->redoIndex] = (char**)malloc(buf->lineCount * sizeof(char*));
        buf->redoLineCount[buf->redoIndex] = buf->lineCount;
        for (int i = 0; i < buf->lineCount; i++) {
            buf->redoBuffer[buf->redoIndex][i] = strdup(buf->lines[i]);
        }
        buf->redoIndex++;
    }

    buf->undoIndex--;
    // Restore state from undo buffer
    for (int i = 0; i < buf->lineCount; i++) {
        free(buf->lines[i]);
    }
    
    buf->lineCount = buf->undoLineCount[buf->undoIndex];
    for (int i = 0; i < buf->lineCount; i++) {
        buf->lines[i] = strdup(buf->undoBuffer[buf->undoIndex][i]);
    }
}

int main(int argc, char* argv[]) {
    Buffer* buf = initBuffer();
    char filename[256] = "le.txt";
    char dirname[256] = ".";
    FILE* fp;

    // Handle command line arguments
    if (argc > 3) {
        printf("Too many arguments\n");
        return 1;
    }

    if (argc >= 2) {
        strcpy(filename, argv[1]);
    }
    
    if (argc == 3) {
        strcpy(dirname, argv[2]);
    }

    // Construct full path
    char filepath[512];
    sprintf(filepath, "%s/%s", dirname, filename);

    // Try to open file in r+ mode first
    fp = fopen(filepath, "r+");
    if (!fp) {
        // If file doesn't exist, create it in w+ mode
        fp = fopen(filepath, "w+");
        if (!fp) {
            printf("Error creating file\n");
            return 1;
        }
    }

    // Read file into buffer
    readFile(buf, fp);
    fclose(fp);

    char command[32];
    char input[MAX_LINE_LENGTH];

    // Main command loop
    while (1) {
        printf("Enter command (search/insert/delete/update/undo/redo/print/save/quit): ");
        scanf("%s", command);

        if (strcmp(command, "quit") == 0) {
            break;
        } else if (strcmp(command, "search") == 0) {
            printf("Enter word to search: ");
            scanf("%s", input);
            buf->cursor = search(buf, input);
            if (buf->cursor.lineNo != -1) {
                printf("Found at line %d, position %d\n", buf->cursor.lineNo + 1, buf->cursor.position);
            } else {
                printf("Word not found\n");
            }
        } else if (strcmp(command, "insert") == 0) {
            printf("Enter line to insert: ");
            getchar(); // Clear newline
            fgets(input, MAX_LINE_LENGTH, stdin);
            input[strcspn(input, "\n")] = 0;
            insertLine(buf, input);
        } else if (strcmp(command, "delete") == 0) {
            deleteLine(buf);
        } else if (strcmp(command, "update") == 0) {
            printf("Enter new text: ");
            getchar(); // Clear newline
            fgets(input, MAX_LINE_LENGTH, stdin);
            input[strcspn(input, "\n")] = 0;
            updateLine(buf, input);
        } else if (strcmp(command, "undo") == 0) {
            undo(buf);
        } else if (strcmp(command, "print") == 0) {
            for (int i = 0; i < buf->lineCount; i++) {
                printf("[Line %d] %s\n", i + 1, buf->lines[i]);
            }
        } else if (strcmp(command, "save") == 0) {
            writeFile(buf, filepath);
            printf("File saved\n");
        }
    }

    // Cleanup
    for (int i = 0; i < buf->lineCount; i++) {
        free(buf->lines[i]);
    }
    free(buf->lines);
    free(buf);

    return 0;
}