module registry;

import std.stdio;
import std.string;
import std.array;
import std.file;
import std.path;
import package_manager; // Dependency ve Package yapıları için

// Kullanılabilir paket ve sürümlerini simüle eden bellek içi veri
// Gerçekte bu bilgi bir API çağrısıyla alınır.
private AvailablePackageVersion[] simulatedRegistryData = [
    { name: "std.stdio", version: "1.0.0", dependencies: [] },
    { name: "std.stdio", version: "1.1.0", dependencies: [] },
    { name: "std.stdio", version: "1.2.0", dependencies: [] },
    { name: "my_library", version: "1.0.0", dependencies: [{name: "another_lib", versionConstraint: "~0.5"}] },
    { name: "my_library", version: "1.1.0", dependencies: [{name: "another_lib", versionConstraint: "~0.6"}] },
    { name: "another_lib", version: "0.5.0", dependencies: [] },
    { name: "another_lib", version: "0.5.1", dependencies: [] },
    { name: "another_lib", version: "0.6.0", dependencies: [] },
];

// Bir paketin belirli bir sürümüne ait bilgiyi registry'den getirmeyi simüle eder
// Gerçekte bir ağ isteği yapar.
AvailablePackageVersion* getPackageInfo(string name, string version) {
    writeln("Registry'den paket bilgisi alınıyor: ", name, " v", version);
    foreach (ref pkg; simulatedRegistryData) {
        if (pkg.name == name && pkg.version == version) {
            writeln("Paket bilgisi bulundu.");
            return &pkg; // Bulunan paketin pointer'ını döndür
        }
    }
    writeln("Paket bilgisi bulunamadı.");
    return null; // Bulunamadı
}

// Bir paketin tüm sürümlerinin bilgisini registry'den getirmeyi simüle eder
// Bağımlılık çözümleyici bu fonksiyonu kullanabilir.
AvailablePackageVersion[] getAllPackageVersions(string name) {
    writeln("Registry'den tüm sürümler alınıyor: ", name);
    AvailablePackageVersion[] versions;
    foreach (pkg; simulatedRegistryData) {
        if (pkg.name == name) {
            versions ~= pkg;
        }
    }
    writeln("Bulunan sürüm sayısı: ", versions.length);
    return versions;
}


// Bir paketin belirli bir sürümünün kaynak kodunu indirmeyi simüle eder
// Gerçekte bir ağ isteği yapar ve dosyaları kaydeder.
// destinationDir: Kaynak kodunun nereye indirileceği.
int downloadPackageSource(string name, string version, string destinationDir) {
    writeln("Paket kaynağı indiriliyor: ", name, " v", version, " -> ", destinationDir);

    // Paketin registry'de olup olmadığını kontrol et
    auto pkgInfo = getPackageInfo(name, version);
    if (!pkgInfo) {
        stderr.writeln("Hata: Registry'de '", name, "' v'", version, "' paketi bulunamadı.");
        return 1;
    }

    // İndirme dizinini oluştur
    try {
        mkdirRec(destinationDir); // Gerekirse üst dizinleri de oluştur
        writeln("İndirme dizini oluşturuldu: ", destinationDir);
    } catch (Exception e) {
        stderr.writeln("Hata: İndirme dizini oluşturulurken hata: ", e.msg);
        return 1;
    }


    // Basit bir simülasyon: Dummy bir kaynak dosyası oluştur
    string dummySourceFileDir = destinationDir ~ "/src";
     try {
        mkdir(dummySourceFileDir);
     } catch (Exception e) {
         stderr.writeln("Hata: Dummy kaynak dizini oluşturulurken hata: ", e.msg);
         return 1;
     }


    string dummySourceFilePath = dummySourceFileDir ~ "/main.dpp"; // Varsayım: Tek main.dpp dosyası var
    try {
        File dummySourceFile = File(dummySourceFilePath, "w");
        scope(exit) dummySourceFile.close();
        dummySourceFile.writeln(`// Bu, ` ~ name ~ ` v` ~ version ~ `'ün dummy kaynak kodudur.`);
        dummySourceFile.writeln(`fn dummy_func() {`);
        dummySourceFile.writeln(`  // ... kod ...`);
        dummySourceFile.writeln(`}`);
         writeln("Dummy kaynak dosyası oluşturuldu: ", dummySourceFilePath);

    } catch (Exception e) {
         stderr.writeln("Hata: Dummy kaynak dosyası yazılırken hata: ", e.msg);
         return 1;
    }


    writeln("Paket kaynağı '", name, "' v'", version, "' başarıyla indirildiği simüle edildi.");
    return 0;
}

// Diğer registry etkileşimleri (paket yayınlama, arama vb.) burada yer alabilir.