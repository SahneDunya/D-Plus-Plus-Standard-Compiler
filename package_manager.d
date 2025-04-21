module package_manager;

import std.stdio;
import std.string;
import std.array;
import std.path;
import std.file;
import dependency_resolver; // dependency_resolver.d dosyasını import ediyoruz
import build_system;      // İleride build_system.d dosyasını import edeceksiniz
import registry;        // İleride registry.d dosyasını import edeceksiniz

// Bir paketi temsil eden basit yapı
struct Package {
    string name;
    string version;
    Dependency[] dependencies; // Bağımlılık listesi
    // Diğer alanlar: yazarlar, lisans, modüller vb.
}

// Bir paket bağımlılığını temsil eden basit yapı
struct Dependency {
    string name;
    string versionConstraint; // Sürüm kısıtlaması (örneğin ">=1.0.0", "~1.2")
}

// Paket yöneticisinin ana fonksiyonu
int runPackageManager(string[] args) {
    if (args.length < 2) {
        printHelp();
        return 1;
    }

    string command = args[1];

    switch (command) {
        case "new":
            if (args.length < 3) {
                stderr.writeln("Kullanım: d++pkg new <paket_adı>");
                return 1;
            }
            string packageName = args[2];
            return createNewPackage(packageName);

        case "add":
            if (args.length < 3) {
                stderr.writeln("Kullanım: d++pkg add <bağımlılık_adı> [sürüm_kısıtlaması]");
                return 1;
            }
            string dependencyName = args[2];
            string versionConstraint = (args.length > 3) ? args[3] : "*"; // Varsayılan kısıtlama "*" (herhangi bir sürüm)
            return addDependency(dependencyName, versionConstraint);

        case "build":
            return buildPackage();

        case "resolve":
             return resolveDependenciesCommand();

        case "help":
            printHelp();
            return 0;

        default:
            stderr.writeln("Hata: Bilinmeyen komut '", command, "'");
            printHelp();
            return 1;
    }
}

// Yardım mesajını yazdırır
void printHelp() {
    writeln("D++ Paket Yöneticisi");
    writeln("Kullanım:");
    writeln("  d++pkg new <paket_adı>      Yeni bir D++ paketi oluşturur.");
    writeln("  d++pkg add <bağımlılık_adı> [sürüm_kısıtlaması]  Pakete bağımlılık ekler.");
    writeln("  d++pkg build              Paketi derler.");
    writeln("  d++pkg resolve            Paket bağımlılıklarını çözer.");
    writeln("  d++pkg help               Bu yardım mesajını gösterir.");
}

// Yeni bir paket dizini ve yapılandırma dosyası oluşturur
int createNewPackage(string name) {
    if (exists(name)) {
        stderr.writeln("Hata: Dizin '", name, "' zaten mevcut.");
        return 1;
    }

    try {
        mkdir(name);
        writeln("Dizin oluşturuldu: ", name);

        // Basit bir paket yapılandırma dosyası (örneğin, JSON veya D syntaxı gibi bir format)
        string configFileName = name ~ "/dpp_package.json"; // Örnek: JSON formatı
        File configFile = File(configFileName, "w");
        scope(exit) configFile.close();

        configFile.writeln("{");
        configFile.writeln(`  "name": "`, name, `",`);
        configFile.writeln(`  "version": "0.1.0",`);
        configFile.writeln(`  "dependencies": []`);
        configFile.writeln("}");

        writeln("Paket yapılandırma dosyası oluşturuldu: ", configFileName);

        // Temel kaynak dosyası oluştur (örneğin, main.dpp)
        string sourceFileName = name ~ "/src/main.dpp"; // Varsayım: Kaynak kodları src/ klasöründe ve uzantı .dpp
        mkdir(name ~ "/src");
        File sourceFile = File(sourceFileName, "w");
        scope(exit) sourceFile.close();
        sourceFile.writeln(`import std.stdio;`);
        sourceFile.writeln(``);
        sourceFile.writeln(`fn main() {`);
        sourceFile.writeln(`  println!("Merhaba, D++!");`);
        sourceFile.writeln(`}`);
        writeln("Örnek kaynak dosyası oluşturuldu: ", sourceFileName);


        writeln("Yeni paket '", name, "' başarıyla oluşturuldu.");
        return 0;

    } catch (Exception e) {
        stderr.writeln("Hata: Yeni paket oluşturulurken bir hata oluştu: ", e.msg);
        // Temizleme işlemleri gerekebilir (oluşturulan dizinleri silme)
        return 1;
    }
}

// Mevcut pakete bağımlılık ekler
int addDependency(string name, string constraint) {
    // Mevcut paket yapılandırma dosyasını bul ve oku (dpp_package.json varsayımı)
    string configFileName = "dpp_package.json"; // Komutun çalıştırıldığı dizinde ara
    if (!exists(configFileName)) {
        stderr.writeln("Hata: Paket yapılandırma dosyası '", configFileName, "' bulunamadı. Bir D++ paketi dizininde olduğunuzdan emin olun veya 'd++pkg new' ile yeni bir paket oluşturun.");
        return 1;
    }

    // Basitlik için dosya içeriğini oku ve elle düzenlemiş gibi yap
    // Gerçekte bir JSON parser kullanılmalıdır.
    string configFileContent = readText(configFileName);

    // Yeni bağımlılık dizesini oluştur
    string newDependencyString = `    { "name": "` ~ name ~ `", "versionConstraint": "` ~ constraint ~ `" }`;

    // "dependencies": [] veya benzer bir yeri bulup bağımlılığı ekle
    // Bu kısım dosya formatına göre değişir ve karmaşıktır.
    // Örneğin, JSON için array içine yeni bir obje eklemek gerekir.
    // Çok basit bir string replace (gerçekten kaçınılması gereken bir yöntem!):
    string updatedContent = configFileContent.replace(`"dependencies": []`, `"dependencies": [\n` ~ newDependencyString ~ `\n  ]`);

    // Eğer zaten bağımlılıklar varsa, son bağımlılıktan sonra virgül ekleyip yeni bağımlılığı ekle
    // Bu da çok kırılgan bir yöntemdir.
     if (updatedContent == configFileContent) {
         // "dependencies": [ ... ] içinde son elemanı bulup sonra eklemeye çalış
         // Bu örnekte implemente etmek çok karmaşık. Gerçek parser şart.
          stderr.writeln("Uyarı: Basit dosya düzenleme başarısız oldu. Lütfen bağımlılığı dpp_package.json dosyasına elle ekleyin.");
          // Yine de temel yapıyı göstermek için dosyayı yazalım (muhtemelen hatalı JSON üretecek)
          try {
             writeText(configFileName, updatedContent); // Hatalı olabilir
             writeln("Paket yapılandırma dosyası güncellenmeye çalışıldı (Elle kontrol edin): ", configFileName);
             writeln("Eklenen bağımlılık: ", name, " (", constraint, ")");
             return 0;
         } catch (Exception e) {
             stderr.writeln("Hata: Paket yapılandırma dosyası yazılırken bir hata oluştu: ", e.msg);
             return 1;
         }
     } else {
         // Başarılı string replace durumunda dosyayı yaz
         try {
             writeText(configFileName, updatedContent);
             writeln("Paket yapılandırma dosyası güncellendi: ", configFileName);
             writeln("Eklenen bağımlılık: ", name, " (", constraint, ")");
             return 0;
         } catch (Exception e) {
             stderr.writeln("Hata: Paket yapılandırma dosyası yazılırken bir hata oluştu: ", e.msg);
             return 1;
         }
     }

     return 0; // Başarılı
}

// Paketi derler (Derleyiciye çağrı yapacak)
int buildPackage() {
    writeln("Paket build ediliyor...");
    // build_system.buildCurrentPackage(); // İleride build_system.d'deki fonksiyonu çağır
    // Burada önce bağımlılıkların çözülmesi ve indirilmesi gerekebilir.
    // Ardından derleyici (compiler.d) uygun argümanlarla çağrılır.
    stderr.writeln("Hata: Build komutu henüz implemente edilmedi."); // Placeholder
    return 1; // Placeholder
}

// Bağımlılıkları çözer ve listeler
int resolveDependenciesCommand() {
    writeln("Bağımlılıklar çözülüyor...");

    // Mevcut paketin bağımlılıklarını yükle (dpp_package.json'dan)
    // Bu kısım da dosya okuma ve parse etme gerektirir.
    // Şimdilik örnek bir paket ve bağımlılıkları kullanalım.
    Package currentPackage;
    currentPackage.name = "my_app";
    currentPackage.version = "0.1.0";
    currentPackage.dependencies = [
        Dependency("std.stdio", "*"), // Örnek bağımlılık: Standart kütüphane I/O modülü
        Dependency("my_library", ">=1.0.0, <2.0.0") // Örnek bağımlılık: Başka bir kütüphane
    ];


    // Bağımlılık çözümleyiciyi çağır
    // Gerçekte, dependency_resolver'ın kullanılabilir paket versiyonları hakkında bilgiye ihtiyacı olacaktır.
    // Bu bilgi bir registry'den (registry.d) gelir.
    ResolvedDependency[] resolved = resolveDependencies(currentPackage.dependencies); // dependency_resolver.d'deki fonksiyon

    if (resolved.length == 0 && currentPackage.dependencies.length > 0) {
        stderr.writeln("Hata: Bağımlılıklar çözülemedi.");
        return 1;
    }

    writeln("Çözümlenen Bağımlılıklar:");
    foreach (dep; resolved) {
        writeln("  - ", dep.name, ": ", dep.version);
    }

    return 0; // Başarılı
}

// Ana giriş noktası
int main(string[] args) {
    // İlk argüman genellikle program adıdır ("d++pkg"), bu yüzden slice alıyoruz.
    return runPackageManager(args[1..$]);
}