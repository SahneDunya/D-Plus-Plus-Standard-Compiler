module parser;

import std.stdio;
import std.array;
import std.exception; // Hatalar için
import syntax_kind; // Token ve AST düğüm türlerini tanımlayan modül
import ast;         // AST düğüm yapılarını tanımlayan modül
import lexer;       // Token yapısı için (veya syntax_kind'den alır)
import error_reporting; // Hata raporlama için (ileride eklenecek)

// Token dizisini ve mevcut pozisyonu tutacak yardımcı yapı
struct TokenStream {
    const(Token)[] tokens; // const: Tokenlar ayrıştırma sırasında değişmez
    int position; // Mevcut pozisyon

    // Stream'den sıradaki token'ı al (pozisyonu ilerletmeden)
    const(Token)* peek(int offset = 0) const {
        int index = position + offset;
        if (index >= 0 && index < tokens.length) {
            return &tokens[index];
        }
        return null; // Stream sonu veya geçersiz offset
    }

    // Stream'den sıradaki token'ı al ve pozisyonu ilerlet
    const(Token) consume() {
        if (position < tokens.length) {
            return tokens[position++];
        }
        // Hata durumu: Stream sonuna ulaşıldı
        // Genellikle EndOfFileToken döndürülür veya hata fırlatılır.
        // Basitlik için burada EndOfFileToken döndürelim (syntax_kind'de tanımlı olmalı)
         return Token(SyntaxKind.EndOfFileToken, "", 0, 0); // Varsayım: EndOfFileToken'ın kind'ı 0
    }

    // Beklenen token türünü tüketir, değilse hata fırlatır
    const(Token) expect(SyntaxKind expectedKind) {
        const(Token) current = peek();
        if (current && current.kind == expectedKind) {
            return consume();
        } else {
            // Hata: Beklenen token türü bulunamadı
             string errorMessage = format("Hata: Satır %s, Sütun %s: '%s' bekleniyor, ancak '%s' bulundu.",
                                        current ? current.lineNumber : 0,
                                        current ? current.columnNumber : 0,
                                        expectedKind.to!string, // SyntaxKind enum'ını stringe çevir (D'nin to!string özelliği)
                                        current ? current.kind.to!string : "Dosya Sonu");

            stderr.writeln(errorMessage);
            // error_reporting.reportError(errorMessage, current ? current.lineNumber : 0, current ? current.columnNumber : 0);
            // Hata kurtarma stratejileri burada devreye girer.
            // Basitlik için burada bir istisna fırlatabiliriz.
            throw new Exception(errorMessage);
            // Veya BadToken tüketip ayrıştırmaya devam etmeye çalışabiliriz.
             return Token(SyntaxKind.BadToken, current ? current.value : "", current ? current.lineNumber : 0, current ? current.columnNumber : 0);
        }
    }
}


// Token listesini alıp AST'nin kök düğümünü döndüren ana parser fonksiyonu
// Gerçek bir parser, dilin gramerine göre recursive descent gibi bir teknik kullanır.
// Burada sadece çok basit bir ifade ayrıştırma örneği gösterilmiştir.
ASTNode* parse(const(Token)[] tokens) {
    writeln("Ayrıştırma aşaması başlatılıyor...");
    TokenStream stream = TokenStream(tokens);

    // AST'nin kök düğümü
    ASTNode* root = new ProgramNode(); // ast.d'de tanımlanmış ProgramNode sınıfını kullanıyoruz

    // Tokenları tüketerek AST ağacını inşa et
    // Bu, dilin gramerine göre recursive fonksiyon çağrıları ile yapılır.
    // Örneğin, bir program bir dizi fonksiyondan oluşur.
    // Her fonksiyon bir bildirim, parametre listesi ve gövdeden oluşur.
    // Gövde bir dizi ifadeden oluşur, ifadeler operasyonlar ve operandlardan oluşur vb.

    // Çok basit bir örnek: Sadece tokenları düz bir listeye ekleyelim (DOĞRU PARSING BU DEĞİLDİR!)
    
    while (stream.peek().kind != SyntaxKind.EndOfFileToken) {
        const(Token) token = stream.consume();
        // Her token için basit bir LiteralExpressionNode gibi düğüm oluştur (yanlış temsil)
        ASTNode* tokenNode = new LiteralExpressionNode(token); // ast.d'de tanımlanacak
        root.addChild(tokenNode);
    }
    */

    // Daha gerçekçi bir başlangıç taslağı: Programın ifadelerden oluştuğunu varsayalım.
    // parseStatement() fonksiyonu bir ifadeyi ayrıştırıp bir AST düğümü döndürür.
    while (stream.peek().kind != SyntaxKind.EndOfFileToken) {
        ASTNode* statement = parseStatement(stream);
        if (statement) {
            root.addChild(statement);
        } else {
             // Hata kurtarma veya durdurma stratejisi
             break; // Basitlik için durdur
        }
    }


    // AST'nin oluşturulup oluşturulmadığını kontrol et
    if (root.children.length == 0 && tokens.length > 1) { // EndOfFileToken hariç token varsa ama AST boşsa
         stderr.writeln("Uyarı: Ayrıştırma sonucunda anlamlı bir AST oluşturulamadı.");
         // Bu durum genellikle gramer veya parser mantığındaki hatalardan kaynaklanır.
    }

    writeln("Ayrıştırma tamamlandı. AST oluşturuldu.");
    return root;
}

// Tek bir ifadeyi ayrıştıran fonksiyon (Çok basit bir taslak)
// Gerçekte burada if, switch, match, döngü, değişken bildirimi, fonksiyon çağrısı gibi
// farklı ifade türlerini ayrıştıran mantık dallanacaktır.
ASTNode* parseStatement(ref TokenStream stream) {
    // Örnek: Eğer sıradaki token bir tanımlayıcıysa ve sonra paren geliyorsa fonksiyon çağrısı olabilir.
    // Eğer "let" anahtar kelimesiyle başlıyorsa değişken bildirimi olabilir.
    // Eğer "if" ile başlıyorsa if ifadesi olabilir.

    const(Token)* current = stream.peek();
    if (!current) return null;

    // Basit bir ifade: identifier ;
    if (current.kind == SyntaxKind.IdentifierToken) {
        const(Token) identifierToken = stream.consume();
        // ast.d'de tanımlanacak IdentifierExpressionNode
        ASTNode* identifierNode = new IdentifierExpressionNode(identifierToken); // Tanımlayıcı düğümü oluştur

        // Eğer sonra noktalı virgül geliyorsa, bu basit bir ifadedir.
        const(Token)* next = stream.peek();
        if (next && next.kind == SyntaxKind.SemicolonToken) {
            stream.consume(); // Noktalı virgülü tüket
            // ast.d'de tanımlanacak ExpressionStatementNode (bir ifadeyi içeren statement)
            return new ExpressionStatementNode(identifierNode); // İfadeyi bir statement içine sar
        } else {
            // Eğer noktalı virgül yoksa, bu daha karmaşık bir ifadenin başlangıcı olabilir (örneğin atama, fonksiyon çağrısı)
            // Bu durumda, duruma göre farklı ayrıştırma fonksiyonları çağrılmalıdır.
            // Şimdilik, basitlik için hata verelim veya sadece tanımlayıcıyı döndürelim.
             stderr.writeln("Uyarı: Satır ", identifierToken.lineNumber, ", Sütun ", identifierToken.columnNumber, ": İfade ayrıştırılamadı (noktalı virgül veya beklenen yapı yok).");
              error_reporting.reportWarning(...);
             return identifierNode; // Eksik bir ifade düğümü döndürmek de bir stratejidir.
        }
    }

    // Örnek: Eğer "let" anahtar kelimesiyse değişken bildirimi ayrıştır.
    if (current.kind == SyntaxKind.LetKeyword) {
         parseVariableDeclaration(stream); // ast.d'de tanımlanacak VariableDeclarationNode döndüren fonksiyon
         stderr.writeln("Uyarı: 'let' anahtar kelimesiyle başlayan ifade ayrıştırma henüz implemente edilmedi.");
         stream.consume(); // En azından "let" tokenını tüket ki sonsuz döngü olmasın
         return null; // Placeholder
    }
    // ... Diğer statement türleri için dallanmalar

    // Bilinmeyen statement başlangıcı
     stderr.writeln("Hata: Satır ", current.lineNumber, ", Sütun ", current.columnNumber, ": Beklenmeyen token ile statement başlangıcı: '", current.value, "' (", current.kind, ")");
      error_reporting.reportError(...);
     stream.consume(); // Bilinmeyen tokenı tüketerek ilerlemeye çalış
     return null; // Geçersiz statement
}

// Diğer ayrıştırma yardımcı fonksiyonları (örneğin parseExpression, parseFunctionDeclaration vb.) burada tanımlanacaktır.
 parseExpression(ref TokenStream stream) { ... } // İfadeleri (aritmetik, mantıksal, fonksiyon çağrıları vb.) ayrıştırır
 parseFunctionDeclaration(ref TokenStream stream) { ... } // Fonksiyon bildirimlerini ayrıştırır
 parseIfStatement(ref TokenStream stream) { ... } // If/else yapılarını ayrıştırır
 parseMatchStatement(ref TokenStream stream) { ... } // Match yapısını ayrıştırır