module io.buffer;

import io; // io.d'deki traitler ve IOError için
import core.result; // Result<T, E> için
import core.option; // Option<T> için
import std.stdio;   // writeln için (debugging)
import std.string;  // format için
import std.array;   // Dizi işlemleri için
import std.conv;    // Tip dönüşümleri için

// Varsayılan tampon boyutu
private enum DEFAULT_BUFFER_SIZE = 8192; // 8 KB

// Tamponlanmış Okuyucu (BufReader)
// R: io.Read traitini implemente eden herhangi bir tür
struct BufReader(R : Read) {
    private R* inner;          // Tamponlanan iç okuyucu (pointer olarak tutalım)
    private ubyte[] buffer;    // Tampon
    private size_t filled;     // Tamponun doluluk oranı
    private size_t consumed;   // Tampondan tüketilen miktar

    // Yeni bir tamponlanmış okuyucu oluşturur
    // reader: Tamponlanacak io.Read nesnesi
    this(R* reader) {
        this.inner = reader;
        this.buffer = new ubyte[DEFAULT_BUFFER_SIZE]; // Varsayılan boyutta tampon oluştur
        this.filled = 0;
        this.consumed = 0;
    }

    // Tamponu iç okuyucudan doldurur
    // Başarılı durumda Ok(okunan_byte_sayısı), hata durumunda Err(IOError) döndürür.
    private Result!(size_t, IOError) fillBuffer() {
        // Tamponu temizle (kullanılan kısmı at)
        buffer[0 .. filled - consumed] = buffer[consumed .. filled];
        filled = filled - consumed;
        consumed = 0;

        // Tamponun kalan kısmını iç okuyucudan doldurmaya çalış
        size_t remaining = buffer.length - filled;
        if (remaining == 0) {
            // Tampon tamamen dolu (normalde fillBuffer çağrılmamalı bu durumda)
            return Result!(size_t, IOError).Ok(0);
        }

        auto readResult = inner.read(buffer[filled .. buffer.length]);

        if (readResult.isOk()) {
            size_t bytesRead = readResult.unwrap();
            filled += bytesRead; // Tamponun doluluk oranını güncelle
            return Result!(size_t, IOError).Ok(bytesRead);
        } else {
            // Okuma hatası
            return Result!(size_t, IOError).Err(readResult.unwrapErr());
        }
    }


    // io.Read traitini implemente et (Tamponlanmış okuma)
    override Result!(size_t, IOError) read(ubyte[] targetBuffer) {
        size_t totalBytesRead = 0;
        size_t targetRemaining = targetBuffer.length;

        while (targetRemaining > 0) {
            // Tamponda okunacak veri var mı?
            size_t bufferedAvailable = filled - consumed;
            if (bufferedAvailable > 0) {
                // Tampondan oku
                size_t bytesToCopy = (bufferedAvailable < targetRemaining) ? bufferedAvailable : targetRemaining;
                targetBuffer[totalBytesRead .. totalBytesRead + bytesToCopy] = buffer[consumed .. consumed + bytesToCopy];
                consumed += bytesToCopy;
                totalBytesRead += bytesToCopy;
                targetRemaining -= bytesToCopy;

                if (targetRemaining == 0) {
                    // Hedef tampon doldu
                    break;
                }
            }

            // Tampon boşaldı, iç okuyucudan doldurmaya çalış
            auto fillResult = fillBuffer();
            if (fillResult.isErr()) {
                // Tampon doldurma hatası
                return Result!(size_t, IOError).Err(fillResult.unwrapErr());
            }
            size_t bytesFilled = fillResult.unwrap();
            if (bytesFilled == 0 && bufferedAvailable == 0) {
                // EOF'a ulaşıldı ve tamponda okunacak bir şey yok
                break;
            }
        }

        return Result!(size_t, IOError).Ok(totalBytesRead);
    }

    override Result!(void, IOError) readExact(ubyte[] buffer) {
        // Tamponlama ile readExact implementasyonu daha karmaşıktır.
        // Hem tampondan okuyup hem de iç okuyucudan eksik kısmı tamamlamalıdır.
        stderr.writeln("Uyarı (Buffer): BufReader.readExact implemente edilmedi.");
         error_reporting.reportWarning(...);
        // Basitlik için hata döndürelim
        return Result!(void, IOError).Err(IOError(IOError.Kind.OtherError, "BufReader.readExact not implemented"));
    }

    // Tampondan bir satır metin okur. Başarılı durumda Ok(string), hata durumunda Err(IOError), EOF durumunda Ok("") döndürebilir.
    // Option<string> de döndürebilir, boş string ile EOF'u ayırt etmek gerekir. Result<Option<string>, IOError> daha iyi olabilir.
    Result!(string, IOError) readLine() {
        string line = "";
        while (true) {
            // Tamponda newline var mı?
            size_t newlinePos = std.array.indexOf(buffer[consumed .. filled], cast(ubyte)'\n');
            if (newlinePos != -1) {
                // Tampondan satırı al
                line ~= cast(string)buffer[consumed .. consumed + newlinePos];
                consumed += newlinePos + 1; // newline'ı da tüket
                return Result!(string, IOError).Ok(line);
            }

            // Tamponda newline yok, tüm tamponu al ve yeni tampon doldur.
            line ~= cast(string)buffer[consumed .. filled];
            consumed = filled; // Tamponu boşaltıldı olarak işaretle

            auto fillResult = fillBuffer();
            if (fillResult.isErr()) {
                return Result!(string, IOError).Err(fillResult.unwrapErr());
            }
            size_t bytesFilled = fillResult.unwrap();
            if (bytesFilled == 0 && line.empty) {
                // EOF'a ulaşıldı ve hiç okunacak veri yoktu
                return Result!(string, IOError).Ok(""); // Boş string ile EOF
            } else if (bytesFilled == 0) {
                 // EOF'a ulaşıldı ama bir miktar veri okuduk
                 // Bu durumda o veriyi bir satır olarak döndürebiliriz.
                 return Result!(string, IOError).Ok(line);
             }
        }
    }

    // Tamponu boşaltır (içindeki okunmamış veriyi atar).
    void discardBuffer() {
        filled = 0;
        consumed = 0;
    }

    // ... Diğer BufReader metotları (read_to_string, bytes, lines iteratorları vb.)
}


// Tamponlanmış Yazıcı (BufWriter)
// W: io.Write traitini implemente eden herhangi bir tür
struct BufWriter(W : Write) {
    private W* inner;          // Tamponlanan iç yazıcı (pointer olarak tutalım)
    private ubyte[] buffer;    // Tampon
    private size_t currentPosition; // Tampondaki mevcut yazma pozisyonu

    // Yeni bir tamponlanmış yazıcı oluşturur
    // writer: Tamponlanacak io.Write nesnesi
    this(W* writer) {
        this.inner = writer;
        this.buffer = new ubyte[DEFAULT_BUFFER_SIZE]; // Varsayılan boyutta tampon oluştur
        this.currentPosition = 0;
    }

    // Tamponu iç yazıcıya boşaltır (flush).
    // Başarılı durumda Ok(()), hata durumunda Err(IOError) döndürür.
    private Result!(void, IOError) flushBuffer() {
        if (currentPosition == 0) {
            return Result!(void, IOError).Ok(void); // Tampon boşsa yapacak bir şey yok
        }

        auto writeResult = inner.writeAll(buffer[0 .. currentPosition]); // Tamponun dolu kısmını yaz
        currentPosition = 0; // Tamponu boşaltıldı olarak işaretle

        if (writeResult.isOk()) {
            return Result!(void, IOError).Ok(void);
        } else {
            // Yazma hatası
            return Result!(void, IOError).Err(writeResult.unwrapErr());
        }
    }

    // io.Write traitini implemente et (Tamponlanmış yazma)
    override Result!(size_t, IOError) write(const(ubyte)[] sourceBuffer) {
        size_t totalBytesWritten = 0;
        size_t sourceRemaining = sourceBuffer.length;

        while (sourceRemaining > 0) {
            size_t bufferRemaining = buffer.length - currentPosition;
            if (bufferRemaining == 0) {
                // Tampon dolu, iç yazıcıya boşalt
                auto flushResult = flushBuffer();
                if (flushResult.isErr()) {
                    return Result!(size_t, IOError).Err(flushResult.unwrapErr());
                }
                // Flush başarılı oldu, tampon artık boş.
                 bufferRemaining = buffer.length;
            }

            // Kaynak tampondaki veriyi iç tampondaki boş yere kopyala
            size_t bytesToCopy = (sourceRemaining < bufferRemaining) ? sourceRemaining : bufferRemaining;
            buffer[currentPosition .. currentPosition + bytesToCopy] = sourceBuffer[totalBytesWritten .. totalBytesWritten + bytesToCopy];
            currentPosition += bytesToCopy; // Tampon yazma pozisyonunu ilerlet
            totalBytesWritten += bytesToCopy; // Toplam yazılan byte sayısını güncelle
            sourceRemaining -= bytesToCopy;

            if (sourceRemaining == 0) {
                // Kaynak tampon tamamen yazıldı
                break;
            }
        }

        return Result!(size_t, IOError).Ok(totalBytesWritten);
    }

    override Result!(void, IOError) writeAll(const(ubyte)[] buffer) {
        // Tüm tamponun yazılmasını sağlamak için write metodunu kullanabiliriz.
        auto writeResult = write(buffer);
        if (writeResult.isOk()) {
            // Yazılan byte sayısı kontrol edilebilir, ancak write metodumuz tümünü yazmaya çalışıyor.
            return Result!(void, IOError).Ok(void);
        } else {
            return Result!(void, IOError).Err(writeResult.unwrapErr());
        }
    }

    override Result!(void, IOError) flush() {
        // Tamponu iç yazıcıya boşalt
        auto flushResult = flushBuffer();
        if (flushResult.isErr()) {
            return Result!(void, IOError).Err(flushResult.unwrapErr());
        }
        // İç yazıcının da flush metodunu çağır (eğer o da tamponluysa veya garanti etmek için)
        return inner.flush(); // İç yazıcının flush sonucunu döndür
    }

    // ... Diğer BufWriter metotları (write_fmt, write_line vb.)

    // Nesne temizlendiğinde tamponu otomatik boşaltmak için destructor
    // Eğer class kullanıyorsak ve GC varsa, destructor çağrılır.
    // Eğer struct kullanıyorsak, explicit flush() veya başka bir mekanizma gerekebilir.
    
    ~this() {
        // Tamponu boşaltmaya çalış (hata raporlamayı nasıl ele alacağız?)
         auto flushResult = flushBuffer();
         if (flushResult.isErr()) {
            stderr.writeln("Uyarı (Buffer): BufWriter destructor'ında flush hatası: ", flushResult.unwrapErr().description());
         }
    }
}