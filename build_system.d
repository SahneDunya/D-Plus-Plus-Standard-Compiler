module build_system;

import std.stdio;
import std.string;
import std.file;
import std.path;
import std.array;
import compiler; // compiler.d modülünü import ediyoruz
import package_manager; // Package ve ResolvedDependency yapıları için

// Bir paketi build eden ana fonksiyon
// Şimdilik ResolvedDependency[] listesini girdi olarak alalım,
// gerçekte package_manager bu listeyi çözdükten sonra burayı çağırır.
int buildPackage(Package packageInfo, ResolvedDependency[] resolvedDependencies) {
    writeln("Paket build ediliyor: ", packageInfo.name, " v", packageInfo.version);

    // Kaynak dosyalarını bulma (Varsayım: src/ dizininde .dpp dosyaları var)
    string sourceDir = packageInfo.name ~ "/src"; // Paket dizini altında src/
    if (!exists(sourceDir) || !isDir(sourceDir)) {
        stderr.writeln("Hata: Kaynak dizini '", sourceDir, "' bulunamadı.");
        return 1;
    }

    string[] sourceFiles;
    try {
        foreach (entry; dirEntries(sourceDir, SpanMode.shallow)) {
            if (entry.isFile() && entry.name.endsWith(".dpp")) { // Sadece .dpp dosyalarını al
                sourceFiles ~= entry.name;
            }
        }
    } catch (Exception e) {
         stderr.writeln("Hata: Kaynak dosyaları aranırken bir hata oluştu: ", e.msg);
         return 1;
    }


    if (sourceFiles.length == 0) {
        stderr.writeln("Uyarı: Kaynak dizininde hiç '.dpp' dosyası bulunamadı.");
        // Hata olarak kabul edilebilir veya boş build yapılabilir duruma göre değişir.
         return 1;
    }

    writeln("Bulunan kaynak dosyalar: ", text(sourceFiles));

    // Derleyici seçeneklerini hazırla
    CompilerOptions options;
    // Basitlik için sadece ilk kaynak dosyayı girdi olarak verelim.
    // Gerçekte tüm kaynak dosyaları derleyiciye gönderilmeli,
    // belki ayrı ayrı derlenip sonra linklenmeli.
    if (sourceFiles.length > 0) {
       options.inputFile = sourceFiles[0]; // Sadece ilk dosyayı işle (Basitlik için)
       options.outputFile = packageInfo.name; // Çıktı dosyasının adı paket adı olsun
    } else {
         stderr.writeln("Hata: Derlenecek kaynak dosya yok.");
         return 1;
    }


    // Bağımlılıkların derleyiciye nasıl iletileceği (include yolları vb.) burada ele alınmalıdır.
    // resolvedDependencies listesi bu aşamada kullanılır.
    // Örneğin, bağımlılıkların kurulu olduğu dizinler derleyicinin include yollarına eklenir.
     options.includePaths = ...;


    // Derleyiciyi çağır
    writeln("Derleyici çağrılıyor...");
    int compileResult = compile(options); // compiler.d'deki compile fonksiyonunu çağır

    if (compileResult != 0) {
        stderr.writeln("Hata: Derleme başarısız oldu.");
        return 1;
    }

    writeln("Paket '", packageInfo.name, "' başarıyla build edildi.");
    return 0;
}

// Build sistemi ile ilgili diğer yardımcı fonksiyonlar burada yer alabilir (test çalıştırma, benchmark vb.)