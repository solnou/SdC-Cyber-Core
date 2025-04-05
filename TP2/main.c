#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

// Importamos la funcion externa de assembler
extern void convert_to_ints(double* in, int* out, int count);

// Estructura para la respuesta de la API en memoria
struct Memory {
    char *data;   // Puntero al buffer de datos
    size_t size;  // Tamaño actual del buffer
};

// Callback para escribir los datos recibidos por libcurl en memoria
static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t new_data = size * nmemb;  // Calcula el tamaño de los nuevos datos
    struct Memory *mem = userp;

    // Realoca memoria para almacenar los nuevos datos
    mem->data = realloc(mem->data, mem->size + new_data + 1);
    if (!mem->data) return 0;  // maneja error

    // Copia los nuevos datos al buffer y actualiza el tamaño
    memcpy(mem->data + mem->size, contents, new_data);
    mem->size += new_data;
    mem->data[mem->size] = '\0';  // Asegura que el buffer termine en un carácter nulo
    return new_data;
}

// Función para extraer los valores "value" del JSON recibido
double* get_gini_values(const char *json, int *count) {
    cJSON *root = cJSON_Parse(json);  
    if (!root) return NULL;  // manejo de errores

    // Obtiene el segundo elemento (donde estan los datos)
    cJSON *arr = cJSON_GetArrayItem(root, 1);
    int n = cJSON_GetArraySize(arr);  // Obtiene el tamaño del arreglo
    double *vals = malloc(n * sizeof(double));  // Reserva memoria para los valores
    int j = 0;

    // Itera y busca los valores "value"
    for (int i = 0; i < n; i++) {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        cJSON *v = cJSON_GetObjectItem(item, "value");
        if (cJSON_IsNumber(v)) vals[j++] = v->valuedouble;  // Almacena los valores numéricos
    }

    cJSON_Delete(root);  // Libera la memoria del JSON
    *count = j;  // Devuelve la cantidad de valores extraídos
    return vals;  // Retorna el arreglo de valores
}

int main() {

    // Inicializa libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    struct Memory mem = { malloc(1), 0 };  // Inicializa la estructura para almacenar la respuesta

    if (!curl) return 1;  // manejo de errores

    // Configura la solicitud HTTPS
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.worldbank.org/v2/en/country/AR/indicator/SI.POV.GINI?format=json&date=2011:2022");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);  // Usa el callback para escribir datos
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);  // Pasa la estructura para almacenar los datos
    curl_easy_perform(curl);  // Realiza la solicitud
    curl_easy_cleanup(curl);  // Limpia la sesión

    int count = 0;
    // Extrae los valores GINI del JSON recibido
    double *gini = get_gini_values(mem.data, &count);
    free(mem.data);  // Libera la memoria del buffer de respuesta
    if (!gini) return 1;  // manejo de errores

    // salida por pantalla con los valores GINI
    printf("Valores GINI encontrados: %d\n", count);

    // Convierte los valores GINI a enteros usando la función en ensamblador
    int *converted = malloc(count * sizeof(int));
    convert_to_ints(gini, converted, count);

    // Imprime los valores convertidos a enteros
    puts("Valores convertidos a enteros:");
    for (int i = 0; i < count; i++) printf("%d ", converted[i]);
    puts("");

    // Libera la memoria reservada
    free(gini);
    free(converted);
    curl_global_cleanup();  // Limpia los recursos globales de libcurl
    return 0;  // Termina el programa exitosamente
}
