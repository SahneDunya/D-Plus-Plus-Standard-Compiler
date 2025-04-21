module backend;

import ir; // Ara Temsil yapılarını import ediyoruz
import std.stdio;
import std.string;
import std.file;

// Hedef platform için kod üretecek ana fonksiyon
// Basitlik için dosya yerine string'e yazdırıyoruz.
// Gerçekte bir dosya stream'ine yazmalısınız.
int generateCode(IrProgram program, string outputFilePath) {
    File outputFile;
    try {
        outputFile = File(outputFilePath, "w");
        writeln("Çıktı dosyası açıldı: ", outputFilePath);
    } catch (FileException e) {
        stderr.writeln("Hata: Çıktı dosyası açılamadı '", outputFilePath, "': ", e.msg);
        return 1; // Hata kodu
    }


    writeln("Kod üretimi aşaması başlatılıyor...");

    // Örnek: Çok basit bir assembly çıktısı formatı (x86-64 syntaxına benzer)
    // Bu sadece demonstrasyon amaçlıdır!
    outputFile.writeln(".global main"); // main fonksiyonunu global yap
    outputFile.writeln(".text");      // Kod bölümü

    foreach (func; program.functions) {
        // Varsayım: Fonksiyon adı IrFunction yapısında tutuluyor
         outputFile.writeln(func.info.name, ":"); // Gerçek kullanımda

        // Basitlik için fonksiyon adını doğrudan yazdıralım (eğer IR'da varsa)
        // Örneğin, IR'da bir "main" fonksiyonu olduğunu varsayalım.
         if (func.blocks.length > 0) {
             // İlk bloğun fonksiyonun başlangıcı olduğunu varsayalım
             // Gerçekte fonksiyon ismi IRProgram veya IrFunction içinde tutulmalıdır.
              if (func.blocks[0].instructions.length > 0 && func.blocks[0].instructions[0].opCode == IrOpCode.Label) {
                 outputFile.writeln("func_", func.blocks[0].instructions[0].labelId, ":"); // Etiketi fonksiyon adı gibi kullan
             } else {
                 // Fonksiyon başlangıcı için varsayılan etiket veya isim
                 outputFile.writeln("function_start:");
             }
         } else {
             outputFile.writeln("empty_function:");
         }


        foreach (block; func.blocks) {
            // Blok etiketini yazdır
            outputFile.writeln(".L", block.id, ":"); // Örnek etiket formatı

            foreach (instruction; block.instructions) {
                // Her IR komutunu hedef assembly koduna çevir
                switch (instruction.opCode) {
                    case IrOpCode.Nop:
                        outputFile.writeln("  nop");
                        break;
                    case IrOpCode.LoadConstant:
                        // Örnek: Sabiti bir kayda yükle
                         mov rax, instruction.constant.intValue // Hedef assembly komutu
                        outputFile.writeln("  ; LoadConstant r", instruction.dest, ", ", instruction.constant.intValue);
                        break;
                    case IrOpCode.LoadVariable:
                        // Örnek: Değişkenin değerini bir kayda yükle
                         mov rax, [rbp - offset] // Hedef assembly komutu
                         outputFile.writeln("  ; LoadVariable r", instruction.dest, ", var_offset_", /* instruction.variable.offset */ 0); // Varsayım: Değişken offset'i var
                        break;
                    case IrOpCode.StoreVariable:
                        // Örnek: Kayıttaki değeri değişkene kaydet
                         mov [rbp - offset], rax // Hedef assembly komutu
                         outputFile.writeln("  ; StoreVariable var_offset_", /* instruction.variable.offset */ 0, ", r", instruction.src1); // Varsayım: Değişken offset'i var
                        break;
                    case IrOpCode.Add:
                        // Örnek: İki kaydı topla ve sonuca yaz
                         add rax, rbx // Hedef assembly komutu (kayıt eşleme yapılacak)
                         outputFile.writeln("  ; Add r", instruction.dest, ", r", instruction.src1, ", r", instruction.src2);
                        break;
                    case IrOpCode.Subtract:
                        // Örnek: Çıkarma
                         outputFile.writeln("  ; Sub r", instruction.dest, ", r", instruction.src1, ", r", instruction.src2);
                        break;
                     case IrOpCode.Call:
                         // Örnek: Fonksiyon çağrısı
                          call function_name // Hedef assembly komutu
                         outputFile.writeln("  ; Call ", /* instruction.function.name */ "some_function"); // Varsayım: Fonksiyon adı var
                         // Parametrelerin nasıl geçirildiği de önemlidir (kayıtlar veya stack)
                         break;
                     case IrOpCode.Return:
                         // Örnek: Fonksiyondan dön
                          mov rax, instruction.src1 // Dönüş değerini uygun kayda koy
                          ret
                         outputFile.writeln("  ; Return r", instruction.src1);
                         outputFile.writeln("  ret"); // Basit dönüş komutu
                         break;
                     case IrOpCode.Label:
                         // Etiket zaten blok başlangıcında yazıldı, burada bir şey yapmaya gerek yok
                         break;
                     case IrOpCode.Jump:
                         // Örnek: Koşulsuz atlama
                          jmp .Ltarget_label_id
                         outputFile.writeln("  jmp .L", instruction.labelId);
                         break;
                     case IrOpCode.BranchIfTrue:
                         // Örnek: Koşullu atlama
                          cmp instruction.src1, 0 // Koşulu kontrol et (basit bir varsayım)
                          jne .Ltarget_label_id
                         outputFile.writeln("  ; BranchIfTrue r", instruction.src1, ", .L", instruction.labelId);
                         // Gerçekte burası koşul türüne ve hedef mimariye göre değişir.
                         break;

                    default:
                        stderr.writeln("Hata: Bilinmeyen IR komutu: ", instruction.opCode);
                         error_reporting.reportError("Bilinmeyen IR komutu", ...);
                        return 1; // Hata kodu
                }
            }
        }
    }


    outputFile.close();
    writeln("Kod üretimi tamamlandı. Çıktı dosyası kapatıldı.");

    return 0; // Başarılı kod
}

// IR programını oluşturacak fonksiyonun imzasını ekleyelim (semantics aşamasından sonra çağrılır)
 IrProgram generateIR(ASTNode* syntaxTree); // Bu fonksiyonun implementasyonu semantics veya ayrı bir IR klasöründe olur.