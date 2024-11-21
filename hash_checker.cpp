#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <magic.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <gtk/gtk.h>

using namespace std;
namespace fs = std::filesystem;

// Función para calcular el hash SHA-256 de un string
string calcularHashSHA256(const string &contenido) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);

  unsigned char *contentBytes = reinterpret_cast<unsigned char *>(
      const_cast<char *>(contenido.c_str()));

  EVP_DigestUpdate(mdctx, contentBytes, contenido.size());
  EVP_DigestFinal_ex(mdctx, hash, NULL);
  EVP_MD_CTX_free(mdctx);

  string hashString;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    char buffer[3];
    sprintf(buffer, "%02x", hash[i]);
    hashString += buffer;
  }

  return hashString;
}

// Función para extraer el contenido de diferentes tipos de archivos usando
// libmagic
string extraerContenido(const string &nombreArchivo) {
  magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
  if (magic_cookie == NULL) {
    cerr << "Error al inicializar libmagic." << endl;
    return "";
  }
  if (magic_load(magic_cookie, NULL) != 0) {
    cerr << "Error al cargar la base de datos de magic: "
         << magic_error(magic_cookie) << endl;
    magic_close(magic_cookie);
    return "";
  }

  const char *mime_type = magic_file(magic_cookie, nombreArchivo.c_str());
  if (mime_type == NULL) {
    cerr << "Error al obtener el tipo MIME del archivo: "
         << magic_error(magic_cookie) << endl;
    magic_close(magic_cookie);
    return "";
  }

  string resultado;
  if (string(mime_type) == "application/pdf") {
    // Extraer texto de PDF usando pdftotext
    string comando = "pdftotext " + nombreArchivo + " -";
    array<char, 128> buffer;
    FILE *pipe = popen(comando.c_str(), "r");
    if (!pipe) {
      cerr << "Error al ejecutar pdftotext." << endl;
      magic_close(magic_cookie);
      return "";
    }
    while (fgets(buffer.data(), 128, pipe) != nullptr) {
      resultado += buffer.data();
    }
    pclose(pipe);
  } else if (string(mime_type) ==
             "application/vnd.openxmlformats-officedocument."
             "wordprocessingml.document") {
    // Extraer texto de DOCX usando docx2txt
    string comando = "docx2txt " + nombreArchivo + " -";
    array<char, 128> buffer;
    FILE *pipe = popen(comando.c_str(), "r");
    if (!pipe) {
      cerr << "Error al ejecutar docx2txt." << endl;
      magic_close(magic_cookie);
      return "";
    }
    while (fgets(buffer.data(), 128, pipe) != nullptr) {
      resultado += buffer.data();
    }
    pclose(pipe);
  } else if (string(mime_type) ==
             "application/vnd.openxmlformats-officedocument."
             "spreadsheetml.sheet") {
    // Extraer texto de XLSX usando xlsx2csv
    string comando = "xlsx2csv " + nombreArchivo + " -";
    array<char, 128> buffer;
    FILE *pipe = popen(comando.c_str(), "r");
    if (!pipe) {
      cerr << "Error al ejecutar xlsx2csv." << endl;
      magic_close(magic_cookie);
      return "";
    }
    while (fgets(buffer.data(), 128, pipe) != nullptr) {
      resultado += buffer.data();
    }
    pclose(pipe);
  } else if (string(mime_type).find("image/") == 0) {
    // Para imágenes, usar un hash del contenido binario
    ifstream archivo(nombreArchivo, ios::binary);
    if (archivo.is_open()) {
      char byte;
      while (archivo.get(byte)) {
        resultado += byte;
      }
      archivo.close();
    } else {
      cerr << "Error al abrir el archivo de imagen." << endl;
    }
  } else if (string(mime_type) == "text/plain" ||
             string(mime_type) == "text/x-c" ||
             string(mime_type) == "text/x-c++" ||
             string(mime_type) == "text/x-python") {
    // Para archivos de texto, C++ y Python, leer el contenido directamente
    ifstream archivo(nombreArchivo);
    if (archivo.is_open()) {
      string linea;
      while (getline(archivo, linea)) {
        resultado += linea + "\n";
      }
      archivo.close();
    } else {
      cerr << "Error al abrir el archivo de texto." << endl;
    }
  } else {
    cout << "Formato de archivo no soportado o archivo no encontrado" << endl;
  }

  magic_close(magic_cookie);
  return resultado;
}

static void calcular_hash(GtkWidget *widget, gpointer data) {
  GtkWidget *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "entry"));
  const gchar *nombreArchivo = gtk_entry_get_text(GTK_ENTRY(entry));

  // Obtener el hash original, la variable primerHash y el tipo MIME original
  string *hashOriginal = static_cast<string*>(g_object_get_data(G_OBJECT(data), "hashOriginal"));
  bool *primerHash = static_cast<bool*>(g_object_get_data(G_OBJECT(data), "primerHash"));
  string *tipoMimeOriginal = static_cast<string*>(g_object_get_data(G_OBJECT(data), "tipoMimeOriginal"));
  bool *esModoOriginal = static_cast<bool*>(g_object_get_data(G_OBJECT(data), "esModoOriginal"));

  string contenido = extraerContenido(nombreArchivo);
  if (!contenido.empty()) {
    // Iniciar el cronómetro
    auto inicio = chrono::high_resolution_clock::now();

    string hash = calcularHashSHA256(contenido);

    // Detener el cronómetro
    auto fin = chrono::high_resolution_clock::now();
    auto duracion = chrono::duration_cast<chrono::milliseconds>(fin - inicio);

    // Obtener el tamaño del archivo
    std::filesystem::path path(nombreArchivo);
    double size = std::filesystem::file_size(path) / 1024.0; // Tamaño en KB

    // Obtener el tipo MIME del archivo actual
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic_cookie, NULL);
    const char *mime_type = magic_file(magic_cookie, nombreArchivo);
    string tipoMimeActual(mime_type);
    magic_close(magic_cookie);

    // Mostrar el hash, tamaño, tiempo y estado en la etiqueta
    GtkWidget *label = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "etiquetaHash"));
    string resultado = "Hash: " + hash + "\n";
    resultado += "Tamaño: " + to_string(size) + " KB\n";
    resultado += "Tiempo: " + to_string(duracion.count()) + " ms\n";

    if (*esModoOriginal) {
      resultado += "Estado: Original\n";
      *hashOriginal = hash; // Guardar el hash original
      *tipoMimeOriginal = tipoMimeActual; // Guardar el tipo MIME original
      *primerHash = true;
      gtk_button_set_label(GTK_BUTTON(widget), "Calcular Hash");
    } else {
      // Verificar si los tipos MIME coinciden
      if (tipoMimeActual != *tipoMimeOriginal) {
        resultado += "Estado: Los formatos de archivo no coinciden\n";
      } else {
        // Comparar el hash con el hash original
        if (*hashOriginal == hash) {
          resultado += "Estado: No modificado\n";
        } else {
          resultado += "Estado: Modificado\n";
        }
      }
    }

    gtk_label_set_text(GTK_LABEL(label), resultado.c_str());
    *esModoOriginal = !(*esModoOriginal); // Cambiar el modo
  }
}

// Función para cambiar el modo entre "original" y "modificado"
static void cambiar_modo(GtkWidget *widget, gpointer data) {
  bool *esModoOriginal = static_cast<bool*>(g_object_get_data(G_OBJECT(data), "esModoOriginal"));
  *esModoOriginal = !(*esModoOriginal); // Cambiar el modo

  GtkWidget *button = GTK_WIDGET(g_object_get_data(G_OBJECT(data), "button"));
  if (*esModoOriginal) {
    gtk_button_set_label(GTK_BUTTON(button), "Calcular Hash ");
  } else {
    gtk_button_set_label(GTK_BUTTON(button), "Calcular Hash ");
  }
}

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *modoButton; // Botón para cambiar el modo

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Hash Checker");
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  entry = gtk_entry_new();
  g_object_set_data(G_OBJECT(window), "entry", entry);

  button = gtk_button_new_with_label("Calcular Hash ");
  g_signal_connect(button, "clicked", G_CALLBACK(calcular_hash), window);
  g_object_set_data(G_OBJECT(window), "button", button); // Guardar el botón

  label = gtk_label_new("Hash:");
  g_object_set_data(G_OBJECT(window), "etiquetaHash", label);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);


  // Inicializar la variable primerHash, hashOriginal y tipoMimeOriginal en la ventana
  bool primerHash = false;
  g_object_set_data(G_OBJECT(window), "primerHash", new bool(primerHash));
  g_object_set_data(G_OBJECT(window), "hashOriginal", new string(""));
  g_object_set_data(G_OBJECT(window), "tipoMimeOriginal", new string(""));

  // Inicializar la variable esModoOriginal en la ventana
  bool esModoOriginal = true;
  g_object_set_data(G_OBJECT(window), "esModoOriginal", new bool(esModoOriginal));

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}