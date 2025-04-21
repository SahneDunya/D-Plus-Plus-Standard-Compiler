module frontend;

import syntax_kind; // İleride token ve AST düğüm türlerini tanımlayan dosya
import ast;         // İleride AST düğüm yapılarını tanımlayan dosya
import std.stdio;
import std.string;
import std.array;

// Basit bir Token yapısı tanımı (Geçici)
struct Token {
    // syntax_kind.SyntaxKind kind; // Token'ın türü (Keyword, Identifier, Operator vb.)
    int kind; // Şimdilik basitçe int kullanalım
    string value; // Token'ın metin değeri (örneğin "if", "myVar", "+")
    int lineNumber; // Kaynak kodundaki satır numarası
    int columnNumber; // Kaynak kodundaki sütun numarası
}

// Basit bir AST düğüm yapısı tanımı (Geçici)
class ASTNode {
    // syntax_kind.SyntaxKind kind; // AST düğümünün türü (FunctionDeclaration, IfStatement, IdentifierExpression vb.)
    int kind; // Şimdilik basitçe int kullanalım
    ASTNode[] children; // Alt düğümler
    string value; // Düğümle ilişkili değer (örneğin, Identifier için isim)

    this(int kind, string value = "") {
        this.kind = kind;
        this.value = value;
    }

    // İleride alt düğüm eklemek için fonksiyonlar eklenebilir
    void addChild(ASTNode child) {
        children ~= child;
    }
}


// Kaynak kodu token'lara ayırma (Lexer)
// Bu sadece çok basit bir örnektir, gerçek bir lexer çok daha karmaşıktır.
Token[] lex(string sourceCode) {
    Token[] tokens;
    int currentPos = 0;
    int lineNumber = 1;
    int columnNumber = 1;

    while (currentPos < sourceCode.length) {
        char currentChar = sourceCode[currentPos];

        // Boşlukları ve satır sonlarını atla
        if (isWhitespace(currentChar)) {
            if (currentChar == '\n') {
                lineNumber++;
                columnNumber = 1;
            } else {
                columnNumber++;
            }
            currentPos++;
            continue;
        }

        // Örnek: Basitçe tanımlayıcıları veya sayıları bulalım
        if (isAlpha(currentChar) || currentChar == '_') {
            string identifier = "";
            while (currentPos < sourceCode.length && (isAlphaNum(sourceCode[currentPos]) || sourceCode[currentPos] == '_')) {
                identifier ~= sourceCode[currentPos];
                currentPos++;
                columnNumber++;
            }
            // tokens ~= Token(syntax_kind.SyntaxKind.Identifier, identifier, lineNumber, columnNumber - identifier.length); // Gerçek kullanımda
            tokens ~= Token(100, identifier, lineNumber, columnNumber - identifier.length); // Geçici: 100 Identifier türü
            continue;
        }

        if (isDigit(currentChar)) {
            string number = "";
            while (currentPos < sourceCode.length && isDigit(sourceCode[currentPos])) {
                number ~= sourceCode[currentPos];
                currentPos++;
                columnNumber++;
            }
             tokens ~= Token(syntax_kind.SyntaxKind.NumberLiteral, number, lineNumber, columnNumber - number.length); // Gerçek kullanımda
            tokens ~= Token(200, number, lineNumber, columnNumber - number.length); // Geçici: 200 Sayı türü
            continue;
        }

        // Örnek: Basitçe bazı operatörleri ve noktalama işaretlerini tanıyalım
        switch (currentChar) {
            case '+': tokens ~= Token(301, "+", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 301 Artı
            case '-': tokens ~= Token(302, "-", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 302 Eksi
            case '(': tokens ~= Token(401, "(", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 401 Sol Parantez
            case ')': tokens ~= Token(402, ")", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 402 Sağ Parantez
            case '{': tokens ~= Token(403, "{", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 403 Sol Küme Parantez
            case '}': tokens ~= Token(404, "}", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 404 Sağ Küme Parantez
            case ';': tokens ~= Token(405, ";", lineNumber, columnNumber); currentPos++; columnNumber++; break; // Geçici: 405 Noktalı Virgül
            // ... diğer operatörler ve noktalama işaretleri
            default:
                // Bilinmeyen karakter durumu
                stderr.writeln("Hata: Tanımlanamayan karakter '", currentChar, "' (", lineNumber, ":", columnNumber, ")");
                currentPos++;
                columnNumber++;
                 error_reporting.reportError("Tanımlanamayan karakter", lineNumber, columnNumber); // İleride kullanılabilir
                break;
        }
    }

     tokens ~= Token(syntax_kind.SyntaxKind.EndOfFile, "", lineNumber, columnNumber); // Gerçek kullanımda
    tokens ~= Token(0, "", lineNumber, columnNumber); // Geçici: 0 Dosya Sonu

    return tokens;
}

// Token listesini alarak AST oluşturan (Parser)
// Bu da çok basit bir örnektir, gerçek bir parser sözdizimi kurallarına göre ağaç inşa eder.
ASTNode* parse(Token[] tokens) {
    if (tokens.length == 0) {
        return null; // Boş token listesi
    }

    // Çok basit bir örnek: İlk token'ı kök düğüm yapalım
    // Gerçekte, dilin gramerine göre karmaşık bir ayrıştırma mantığı burada olur.
    // Örneğin, recursive descent veya başka bir parsing tekniği kullanılır.

    // Varsayalım ki basit bir ifade ayrıştırıyoruz: identifier + identifier ;
    // Gerçek AST, dilin yapısını yansıtmalıdır (fonksiyon bildirimleri, if ifadeleri vb.)

    ASTNode* root = new ASTNode(500, "Program"); // Geçici: 500 Program türü

    int i = 0;
    while (i < tokens.length && tokens[i].kind != 0) { // 0: EndOfFile
        // Burada token'ları işleyip AST düğümleri oluşturmalısınız.
        // Örnek: Basitçe her token için bir düğüm ekleyelim (çok yanlış bir yaklaşım ama örnek için)
        // Gerçekte, dilin gramerine göre hiyerarşik bir yapı oluşturulur.
        // Örneğin, bir fonksiyon bildirimini tanıyıp, bunun için bir AST düğümü oluşturup,
        // parametreleri ve gövdesini alt düğümler olarak eklemelisiniz.

        // Basit bir ifade ayrıştırma örneği taslağı: identifier + identifier ;
        
        if (tokens[i].kind == 100) { // Identifier
            ASTNode* identifierNode = new ASTNode(501, tokens[i].value); // Geçici: 501 IdentifierExpression
            root.addChild(identifierNode);
            i++;
            if (i < tokens.length && tokens[i].kind == 301) { // +
                 ASTNode* operatorNode = new ASTNode(502, tokens[i].value); // Geçici: 502 BinaryOperator
                 // Gerçekte burada ağacın yapısı değişir, operatörün sol ve sağ işlenenleri olur.
                 // Örneğin: BinaryOperatorNode { left: IdentifierNode, right: IdentifierNode }
                 root.addChild(operatorNode);
                 i++;
                 if (i < tokens.length && tokens[i].kind == 100) { // Identifier
                     ASTNode* identifierNode2 = new ASTNode(501, tokens[i].value);
                     root.addChild(identifierNode2);
                     i++;
                     if (i < tokens.length && tokens[i].kind == 405) { // ;
                         ASTNode* semicolonNode = new ASTNode(405, tokens[i].value);
                         root.addChild(semicolonNode);
                         i++;
                         writeln("Basit ifade ayrıştırıldı: identifier + identifier ;");
                         continue; // Sonraki ifadeye geç (eğer varsa)
                     }
                 }
            }
        }
        
        // Yukarıdaki basit ifade ayrıştırma taslağı bile gerçek bir grameri yansıtmaz,
        // sadece parsing mantığının ne kadar karmaşık olabileceğini göstermek içindir.
        // Gerçekte, dilin BNF veya EBNF gramerine dayalı bir parser yazılmalıdır.

        // Şimdilik sadece token'ları düz bir liste olarak ekleyelim (çok temel bir temsil)
         ASTNode* tokenNode = new ASTNode(tokens[i].kind, tokens[i].value);
         root.addChild(tokenNode);
         i++;
    }


    // EndOfFile token'ını da ekleyelim (isteğe bağlı)
     root.addChild(new ASTNode(tokens[i].kind, tokens[i].value));


    return root;
}

// isWhitespace, isAlpha, isAlphaNum, isDigit gibi yardımcı fonksiyonlar std.ascii'den gelebilir veya burada tanımlanabilir.
bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isAlphaNum(char c) {
    return isAlpha(c) || isDigit(c);
}

// AST'yi temizlemek için bir fonksiyon (Bellek sızıntısını önlemek için önemlidir)
// class ASTNode kullanıyorsanız çöp toplama yardımcı olacaktır, ancak pointer kullanıyorsanız dikkatli olmalısınız.
void destroyAST(ASTNode* node) {
    if (!node) return;
    foreach (child; node.children) {
        destroyAST(child);
    }
    destroy(node);
}