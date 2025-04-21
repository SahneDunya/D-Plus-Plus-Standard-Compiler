module lexer;

import std.stdio;
import std.string;
import std.array;
import std.ascii; // Karakter kontrol fonksiyonları için
import syntax_kind; // Token türlerini tanımlayan modül (SyntaxKind enum'ı)
import error_reporting; // Hata raporlama için (ileride eklenecek)

// Belirteci (Token) temsil eden yapı
// syntax_kind.d dosyasında da tanımlanabilir ve burada import edilebilir.
struct Token {
    SyntaxKind kind; // Belirtecin türü (örn: Identifier, NumberLiteral, Plus)
    string value;   // Belirtecin metin değeri (örn: "myVar", "123", "+")
    int lineNumber; // Kaynak kodundaki satır numarası
    int columnNumber; // Kaynak kodundaki sütun numarası
}

// Kaynak kodu alıp token dizisi döndüren ana lexer fonksiyonu
Token[] lex(string sourceCode) {
    Token[] tokens;
    int currentPos = 0; // Kaynak kodunda mevcut pozisyon
    int lineNumber = 1; // Mevcut satır numarası
    int columnNumber = 1; // Mevcut sütun numarası

    while (currentPos < sourceCode.length) {
        char currentChar = sourceCode[currentPos];

        // 1. Boşlukları ve yeni satır karakterlerini atla
        if (isWhitespace(currentChar)) {
            if (currentChar == '\n') {
                lineNumber++;
                columnNumber = 1; // Yeni satırda sütun sıfırlanır
            } else {
                columnNumber++;
            }
            currentPos++;
            continue; // Sonraki karaktere geç
        }

        // 2. Tek karakterlik tokenları işle (Operatörler, Ayraçlar, Noktalama İşaretleri)
        switch (currentChar) {
            case '+': tokens ~= Token(SyntaxKind.PlusToken, "+", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '-': tokens ~= Token(SyntaxKind.MinusToken, "-", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '*': tokens ~= Token(SyntaxKind.StarToken, "*", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '/': tokens ~= Token(SyntaxKind.SlashToken, "/", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '(': tokens ~= Token(SyntaxKind.OpenParenToken, "(", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case ')': tokens ~= Token(SyntaxKind.CloseParenToken, ")", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '{': tokens ~= Token(SyntaxKind.OpenBraceToken, "{", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '}': tokens ~= Token(SyntaxKind.CloseBraceToken, "}", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '[': tokens ~= Token(SyntaxKind.OpenBracketToken, "[", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case ']': tokens ~= Token(SyntaxKind.CloseBracketToken, "]", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case ';': tokens ~= Token(SyntaxKind.SemicolonToken, ";", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case ',': tokens ~= Token(SyntaxKind.CommaToken, ",", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            case '.': tokens ~= Token(SyntaxKind.DotToken, ".", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
            // ... Diğer tek karakterlik tokenlar
        }

        // 3. İki veya daha fazla karakterlik tokenları işle (Örnek: ==, !=, <=, >=, ->)
        // Bu kısım currentChar'dan sonraki karaktere bakmayı gerektirir.
        // Örneğin, eğer currentChar == '=' ise, sonraki karakterin de '=' olup olmadığını kontrol et.
         switch (currentChar) {
             case '=':
                 if (currentPos + 1 < sourceCode.length && sourceCode[currentPos + 1] == '=') {
                     tokens ~= Token(SyntaxKind.EqualsEqualsToken, "==", lineNumber, columnNumber); currentPos += 2; columnNumber += 2; continue;
                 } else {
                     tokens ~= Token(SyntaxKind.EqualsToken, "=", lineNumber, columnNumber); currentPos++; columnNumber++; continue;
                 }
             // ... Diğer çok karakterli tokenlar
         }


        // 4. Tanımlayıcıları ve Anahtar Kelimeleri işle
        if (isAlpha(currentChar) || currentChar == '_') {
            string identifier = "";
            int startColumn = columnNumber;
            while (currentPos < sourceCode.length && (isAlphaNum(sourceCode[currentPos]) || sourceCode[currentPos] == '_')) {
                identifier ~= sourceCode[currentPos];
                currentPos++;
                columnNumber++;
            }

            // Tanımlanan metin bir anahtar kelime mi kontrol et
            SyntaxKind keywordKind = getKeywordKind(identifier); // syntax_kind.d'de tanımlanacak fonksiyon
            if (keywordKind != SyntaxKind.UnknownToken) { // Eğer bir anahtar kelimeyse
                tokens ~= Token(keywordKind, identifier, lineNumber, startColumn);
            } else { // Değilse, bu bir tanımlayıcıdır
                tokens ~= Token(SyntaxKind.IdentifierToken, identifier, lineNumber, startColumn);
            }
            continue;
        }

        // 5. Sayısal Literalleri işle (Tam sayılar, ondalıklı sayılar)
        if (isDigit(currentChar)) {
            string number = "";
            int startColumn = columnNumber;
            bool isFloatingPoint = false; // Ondalıklı sayı bayrağı

            while (currentPos < sourceCode.length && isDigit(sourceCode[currentPos])) {
                number ~= sourceCode[currentPos];
                currentPos++;
                columnNumber++;
            }

            // Ondalık kısmı kontrol et
            if (currentPos < sourceCode.length && sourceCode[currentPos] == '.') {
                isFloatingPoint = true;
                number ~= sourceCode[currentPos];
                currentPos++;
                columnNumber++;
                while (currentPos < sourceCode.length && isDigit(sourceCode[currentPos])) {
                    number ~= sourceCode[currentPos];
                    currentPos++;
                    columnNumber++;
                }
            }

            // Üstel kısmı kontrol et (örneğin 1.2e+3) - karmaşıktır, atlandı.

            if (isFloatingPoint) {
                tokens ~= Token(SyntaxKind.FloatingPointLiteralToken, number, lineNumber, startColumn);
            } else {
                tokens ~= Token(SyntaxKind.IntegerLiteralToken, number, lineNumber, startColumn);
            }
            continue;
        }

        // 6. String Literalleri işle (Çift tırnak içindeki metinler)
        if (currentChar == '"') {
            string strValue = "";
            int startColumn = columnNumber;
            currentPos++; columnNumber++; // Açılış tırnağını atla

            while (currentPos < sourceCode.length && sourceCode[currentPos] != '"') {
                // Kaçış karakterlerini (\n, \t, \\, \" vb.) burada işlemeniz gerekir.
                if (sourceCode[currentPos] == '\\') {
                    // Kaçış karakteri işleme mantığı (karmaşık)
                    strValue ~= sourceCode[currentPos]; // Şimdilik ham ekleyelim
                    currentPos++; columnNumber++;
                    if (currentPos < sourceCode.length) {
                         strValue ~= sourceCode[currentPos];
                         currentPos++; columnNumber++;
                    }
                } else {
                    strValue ~= sourceCode[currentPos];
                    currentPos++;
                    columnNumber++;
                }
            }

            if (currentPos < sourceCode.length && sourceCode[currentPos] == '"') {
                currentPos++; columnNumber++; // Kapanış tırnağını atla
                tokens ~= Token(SyntaxKind.StringLiteralToken, strValue, lineNumber, startColumn);
            } else {
                // Hata: Kapanış tırnağı bulunamadı
                 stderr.writeln("Hata: Satır ", lineNumber, ", Sütun ", startColumn, ": Kapanış tırnağı bekleniyor.");
                 // error_reporting.reportError("Kapanış tırnağı bekleniyor", lineNumber, startColumn);
                 // Bu token'ı hata token'ı olarak ekleyebilir veya işlemi durdurabilirsiniz.
                 tokens ~= Token(SyntaxKind.BadToken, sourceCode[startColumn .. currentPos], lineNumber, startColumn); // Hatalı token'ı ekle
            }
            continue;
        }

        // 7. Yorum Satırlarını işle (Örnek: // tek satırlık, /* */ çok satırlık)
        // Tek satırlık yorum
        if (currentChar == '/' && currentPos + 1 < sourceCode.length && sourceCode[currentPos + 1] == '/') {
            while (currentPos < sourceCode.length && sourceCode[currentPos] != '\n') {
                currentPos++;
                columnNumber++;
            }
            continue; // Yorumu atla
        }
        // Çok satırlık yorum (karmaşık, atlandı)
         if (currentChar == '/' && currentPos + 1 < sourceCode.length && sourceCode[currentPos + 1] == '*') { ... }


        // 8. Bilinmeyen karakter durumu
        stderr.writeln("Hata: Satır ", lineNumber, ", Sütun ", columnNumber, ": Tanımlanamayan karakter '", currentChar, "'");
         error_reporting.reportError("Tanımlanamayan karakter", lineNumber, columnNumber);
        tokens ~= Token(SyntaxKind.BadToken, text(currentChar), lineNumber, columnNumber); // Hatalı token olarak ekle
        currentPos++;
        columnNumber++;
    }

    // Dosya sonu belirtecini ekle
    tokens ~= Token(SyntaxKind.EndOfFileToken, "", lineNumber, columnNumber);

    writeln("Tarama tamamlandı. Toplam token sayısı: ", tokens.length);
    // Basitçe tokenları yazdıralım (debugging için faydalı)
    
    foreach (t; tokens) {
        writeln("Kind: ", t.kind, ", Value: \"", t.value, "\", Pos: ", t.lineNumber, ":", t.columnNumber);
    }
    
    return tokens;
}


// Yardımcı fonksiyon: Bir stringin anahtar kelime olup olmadığını kontrol eder ve türünü döndürür.
// syntax_kind.d içinde de yer alabilir.
SyntaxKind getKeywordKind(string text) {
    final switch (text) {
        case "if": return SyntaxKind.IfKeyword;
        case "else": return SyntaxKind.ElseKeyword;
        case "switch": return SyntaxKind.SwitchKeyword;
        case "match": return SyntaxKind.MatchKeyword; // D++'a özel
        case "fn": return SyntaxKind.FnKeyword; // D++'a özel (fonksiyon tanımı için)
        case "let": return SyntaxKind.LetKeyword; // D++'a özel (değişken bildirimi için)
        case "mut": return SyntaxKind.MutKeyword; // D++'a özel (değişkenin değiştirilebilirliği için)
        case "struct": return SyntaxKind.StructKeyword;
        case "class": return SyntaxKind.ClassKeyword;
        case "enum": return SyntaxKind.EnumKeyword;
        case "import": return SyntaxKind.ImportKeyword;
        case "return": return SyntaxKind.ReturnKeyword;
        // ... Diğer D++ anahtar kelimeleri
        default: return SyntaxKind.UnknownToken; // Anahtar kelime değil
    }
}