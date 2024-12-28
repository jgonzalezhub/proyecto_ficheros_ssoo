#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cabeceras.h"
#define LONGITUD_COMANDO 100

//Cambiar el nombre a como pone en la practica
//Cabeceras.h dejarlo como en el zip
//El otro archivo es el disco duro, no tocarlo

// Declaración de funciones
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);//bytemaps
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);//info
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);//dir
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);//rename
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);//imprimir
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre);//remove
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino);//copy
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);//Salir
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);

int main() {
    char comando[LONGITUD_COMANDO];
    char orden[LONGITUD_COMANDO];
    char argumento1[LONGITUD_COMANDO];
    char argumento2[LONGITUD_COMANDO];

    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
    
	int grabardatos = 0;

	// Abrir archivo binario en modo lectura-escritura
    FILE *fent;
    fent = fopen("particion.bin", "r+b");
    if (fent == NULL) {
        printf("Error: No se pudo abrir el archivo de la partición.\n");
        return -1;
    }
	fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);

    // Copia de datos del fichero a las estructuras
    memcpy(&ext_superblock, (EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&ext_bytemaps, (EXT_BYTE_MAPS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos, (EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(&directorio, (EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(&memdatos, (EXT_DATOS *)&datosfich[4], MAX_BLOQUES_DATOS * SIZE_BLOQUE);

    // Bucle de comandos
    for (;;) {
        printf(">> ");
        fflush(stdin);
        fgets(comando, LONGITUD_COMANDO, stdin);
        
		// Comprobar el comando introducido
		if (ComprobarComando(comando, orden, argumento1, argumento2) != 0) {
            printf("Error comando ilegal. Comandos válidos: info, bytemaps, dir, rename, imprimir, remove, copy, salir\n");
            continue;
        }

        if (strcmp(orden, "info") == 0) {
            LeeSuperBloque(&ext_superblock);
            continue;
        }

        if (strcmp(orden, "bytemaps") == 0) {
            Printbytemaps(&ext_bytemaps);
            continue;
        }

        if (strcmp(orden, "dir") == 0) {
            Directorio(directorio, &ext_blq_inodos);
            continue;
        }

        if (strcmp(orden, "rename") == 0) {
            Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2);
            continue;
        }

        if (strcmp(orden, "imprimir") == 0) {
            Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
            continue;
        }

        if (strcmp(orden, "remove") == 0) {
            Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1);
            continue;
        }

        if (strcmp(orden, "copy") == 0) {
            Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2);
            continue;
        }

        if (strcmp(orden, "salir") == 0) {
            GrabarDatos(memdatos, fent);
            fclose(fent);
            printf("Sesión terminada.\n");
            return 0;
        }

        printf("Error comando ilegal. Comandos válidos: info, bytemaps, dir, rename, imprimir, remove, copy, salir\n");
    }
}

// Función para comprobar comandos y argumentos
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    int n = sscanf(strcomando, "%s %s %s", orden, argumento1, argumento2);
    if (n < 1) return -1;  // Comando inválido
    return 0;
}

// Mostrar información del superbloque
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    printf("Información del superbloque:\n");
    printf("Inodos totales: %u\n", psup->s_inodes_count);
    printf("Bloques totales: %u\n", psup->s_blocks_count);
    printf("Bloques libres: %u\n", psup->s_free_blocks_count);
    printf("Inodos libres: %u\n", psup->s_free_inodes_count);
    printf("Primer bloque de datos: %u\n", psup->s_first_data_block);
    printf("Tamaño del bloque: %u bytes\n", psup->s_block_size);
}

// Mostrar el contenido de los bytemaps
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    printf("Bytemap de inodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\nBytemap de bloques (primeros 25):\n");
    for (int i = 0; i < 25; i++) {
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");
}

// Función para mostrar el directorio
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    printf("Listado de archivos:\n");
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            printf("Nombre: %s, Inodo: %d, Tamaño: %d bytes\n", directorio[i].dir_nfich, directorio[i].dir_inodo,
                   inodos->blq_inodos[directorio[i].dir_inodo].size_fichero);
            printf("Bloques: ");
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                printf("%d ", inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j]);
            }
            printf("\n");
        }
    }
}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    int i, existe_origen = -1, existe_destino = -1;

    // Buscar si existen los nombres en el directorio
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
            existe_origen = i; // Encontramos el nombre original
        }
        if (strcmp(directorio[i].dir_nfich, nombrenuevo) == 0) {
            existe_destino = i; // El nombre nuevo ya existe
        }
    }

    if (existe_origen == -1) {
        printf("Error: El archivo '%s' no existe.\n", nombreantiguo);
        return -1;
    }
    if (existe_destino != -1) {
        printf("Error: El nombre '%s' ya está en uso.\n", nombrenuevo);
        return -1;
    }

    // Renombrar el archivo
    strcpy(directorio[existe_origen].dir_nfich, nombrenuevo);
    printf("Archivo '%s' renombrado a '%s'.\n", nombreantiguo, nombrenuevo);
    return 0;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    int i, j, inodo_index = -1;

    // Buscar el archivo en el directorio
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            inodo_index = directorio[i].dir_inodo;
            break;
        }
    }

    if (inodo_index == -1) {
        printf("Error: El archivo '%s' no existe.\n", nombre);
        return -1;
    }

    // Obtener el inodo del archivo
    EXT_SIMPLE_INODE inodo = inodos->blq_inodos[inodo_index];
    printf("Contenido del archivo '%s':\n", nombre);

    // Imprimir los bloques de datos del archivo
    for (j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo.i_nbloque[j] != NULL_BLOQUE; j++) {
        fwrite(memdatos[inodo.i_nbloque[j]].dato, 1, SIZE_BLOQUE, stdout);
    }
    printf("\n");
    return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre) {
    int i, j, inodo_index = -1;

    // Buscar el archivo en el directorio
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            inodo_index = directorio[i].dir_inodo;
            break;
        }
    }

    if (inodo_index == -1) {
        printf("Error: El archivo '%s' no existe.\n", nombre);
        return -1;
    }

    // Liberar bloques de datos en el bytemap
    for (j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        if (inodos->blq_inodos[inodo_index].i_nbloque[j] != NULL_BLOQUE) {
            ext_bytemaps->bmap_bloques[inodos->blq_inodos[inodo_index].i_nbloque[j]] = 0;
            inodos->blq_inodos[inodo_index].i_nbloque[j] = NULL_BLOQUE;
            ext_superblock->s_free_blocks_count++;
        }
    }

    // Marcar el inodo como libre
    ext_bytemaps->bmap_inodos[inodo_index] = 0;
    inodos->blq_inodos[inodo_index].size_fichero = 0;
    ext_superblock->s_free_inodes_count++;

    // Eliminar la entrada del directorio
    strcpy(directorio[i].dir_nfich, "");
    directorio[i].dir_inodo = NULL_INODO;

    printf("Archivo '%s' eliminado.\n", nombre);
    return 0;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino) {
    int i, j, inodo_origen = -1, inodo_destino = -1, nuevo_inodo = -1;

    // Buscar el archivo origen
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreorigen) == 0) {
            inodo_origen = directorio[i].dir_inodo;
        }
        if (strcmp(directorio[i].dir_nfich, nombredestino) == 0) {
            printf("Error: El archivo destino '%s' ya existe.\n", nombredestino);
            return -1;
        }
    }

    if (inodo_origen == -1) {
        printf("Error: El archivo origen '%s' no existe.\n", nombreorigen);
        return -1;
    }

    // Buscar un inodo libre
    for (i = 0; i < MAX_INODOS; i++) {
        if (ext_bytemaps->bmap_inodos[i] == 0) {
            nuevo_inodo = i;
            break;
        }
    }
    if (nuevo_inodo == -1) {
        printf("Error: No hay inodos libres.\n");
        return -1;
    }

    // Crear la nueva entrada en el directorio
    for (i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            strcpy(directorio[i].dir_nfich, nombredestino);
            directorio[i].dir_inodo = nuevo_inodo;
            break;
        }
    }

    // Copiar inodo y bloques
    EXT_SIMPLE_INODE *inodo_orig = &inodos->blq_inodos[inodo_origen];
    EXT_SIMPLE_INODE *inodo_dest = &inodos->blq_inodos[nuevo_inodo];
    *inodo_dest = *inodo_orig;

    for (j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        if (inodo_orig->i_nbloque[j] != NULL_BLOQUE) {
            // Encontrar un bloque libre
            for (int k = 0; k < MAX_BLOQUES_PARTICION; k++) {
                if (ext_bytemaps->bmap_bloques[k] == 0) {
                    ext_bytemaps->bmap_bloques[k] = 1;
                    ext_superblock->s_free_blocks_count--;
                    inodo_dest->i_nbloque[j] = k;
                    memcpy(memdatos[k].dato, memdatos[inodo_orig->i_nbloque[j]].dato, SIZE_BLOQUE);
                    break;
                }
            }
        }
    }

    ext_bytemaps->bmap_inodos[nuevo_inodo] = 1;
    ext_superblock->s_free_inodes_count--;
    printf("Archivo '%s' copiado a '%s'.\n", nombreorigen, nombredestino);
    return 0;
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    // Moverse al inicio del bloque 2 para los inodos
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);
    fwrite(inodos, SIZE_BLOQUE, 1, fich);

    // Moverse al inicio del bloque 3 para el directorio
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);
    fwrite(directorio, SIZE_BLOQUE, 1, fich);

    fflush(fich); // Forzar a que los datos se escriban en disco
    printf("Inodos y directorio grabados correctamente.\n");
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    // Moverse al inicio del bloque 1 para el bytemap
    fseek(fich, SIZE_BLOQUE * 1, SEEK_SET);
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);

    fflush(fich); // Forzar a que los datos se escriban en disco
    printf("Bytemaps grabados correctamente.\n");
}


void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    // Moverse al inicio del bloque 0 para el superbloque
    fseek(fich, SIZE_BLOQUE * 0, SEEK_SET);
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);

    fflush(fich); // Forzar a que los datos se escriban en disco
    printf("Superbloque grabado correctamente.\n");
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    // Moverse al inicio del bloque 4 (primer bloque de datos)
    fseek(fich, SIZE_BLOQUE * PRIM_BLOQUE_DATOS, SEEK_SET);
    fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);

    fflush(fich); // Forzar a que los datos se escriban en disco
    printf("Bloques de datos grabados correctamente.\n");
}








