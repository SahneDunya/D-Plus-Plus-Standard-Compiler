module ast;

import syntax_kind; // SyntaxKind enum'ı için
import lexer;       // Token yapısı için (veya syntax_kind'den alınabilir)
import std.array;

// Tüm AST düğümleri için temel sınıf
// Genellikle sanal fonksiyonlar (örneğin, accept(Visitor visitor) gibi) içerir.
// polymorphism için sınıf kullanmak daha yaygındır.
abstract class ASTNode {
    SyntaxKind kind; // Bu düğümün türü
    // Hata raporlama veya debugging için konum bilgisi de burada tutulabilir.
     int lineNumber;
     int columnNumber;

    this(SyntaxKind kind) {
        this.kind = kind;
    }

    // Bu düğümün alt düğümlerini döndüren sanal fonksiyon
    // Her alt sınıf kendi alt düğümlerini döndürmek için bunu implement eder.
     abstract ASTNode[] getChildren(); // Gelişmiş AST yapılarında kullanılır

    // Temel bir alt düğüm listesi (daha basit ağaçlar için)
    ASTNode[] children;

    void addChild(ASTNode child) {
        if (child) { // Null çocukları eklememek iyi bir pratiktir
            this.children ~= child;
        }
    }

    // AST'yi yazdırmak veya görselleştirmek için yardımcı fonksiyonlar (debugging için)
    void dump(string indent = "") const {
        stdout.write(indent, kind.to!string);
        if (!value.empty) {
            stdout.write(" (", value, ")");
        }
        stdout.writeln();
        // Gelişmiş AST'de getChildren() kullanılır. Basit yapıda children dizisini kullanabiliriz.
        foreach (child; children) {
            child.dump(indent ~ "  "); // İki boşluk girinti ekle
        }
    }

    // Bazı düğümlerde ekstra değer olabilir (örneğin, IdentifierNode'da isim)
    // Bu değeri tutmak için basit bir alan ekleyelim (tüm düğümlerde kullanılmayabilir)
    string value; // Token'ın değeri veya düğümle ilişkili başka bir string bilgi

}

// Programın kök düğümü
class ProgramNode : ASTNode {
    this() {
        super(SyntaxKind.ProgramNode);
    }

    // override ASTNode[] getChildren() { return declarations; } // Gelişmiş AST

    // Programın içerdiği üst düzey bildirimler (fonksiyonlar, structlar vb.)
     ASTNode[] declarations; // Gelişmiş AST
     void addDeclaration(ASTNode decl) { declarations ~= decl; } // Gelişmiş AST
}

// Fonksiyon Bildirimi düğümü (Örnek)
class FunctionDeclarationNode : ASTNode {
    IdentifierExpressionNode* name; // Fonksiyon adı
    ParameterNode[] parameters;     // Parametre listesi
    TypeNode* returnType;           // Dönüş tipi
    BlockStatementNode* body;       // Fonksiyon gövdesi (bir blok statement)

    this(IdentifierExpressionNode* name, ParameterNode[] parameters, TypeNode* returnType, BlockStatementNode* body) {
        super(SyntaxKind.FunctionDeclarationNode);
        this.name = name;
        this.parameters = parameters;
        this.returnType = returnType;
        this.body = body;

        // Temel sınıftaki children listesini de doldurabiliriz (debugging veya basit travers için)
        addChild(name);
        foreach (param; parameters) {
            addChild(param);
        }
        addChild(returnType);
        addChild(body);
    }

     override ASTNode[] getChildren() { ... return combined list ... } // Gelişmiş AST
}

// Değişken Bildirimi düğümü (Örnek: let mut x: int = 5;)
class VariableDeclarationNode : ASTNode {
    bool isMutable; // 'mut' kullanıldı mı?
    IdentifierExpressionNode* name; // Değişken adı
    TypeNode* type;       // Tip belirtimi (isteğe bağlı olabilir)
    ASTNode* initializer; // Başlangıç değeri ataması (ifade düğümü)

    this(bool isMutable, IdentifierExpressionNode* name, TypeNode* type, ASTNode* initializer) {
        super(SyntaxKind.VariableDeclarationNode);
        this.isMutable = isMutable;
        this.name = name;
        this.type = type;
        this.initializer = initializer;

        addChild(name);
        addChild(type); // type null olabilir
        addChild(initializer); // initializer null olabilir
    }
}


// İfadeyi temsil eden temel soyut sınıf
abstract class ExpressionNode : ASTNode {
    this(SyntaxKind kind) {
        super(kind);
    }
}

// Tanımlayıcı Kullanımı (Değişken veya fonksiyon adı)
class IdentifierExpressionNode : ExpressionNode {
    const(Token) identifierToken; // İlgili IdentifierToken

    this(const(Token) identifierToken) {
        super(SyntaxKind.IdentifierExpressionNode);
        this.identifierToken = identifierToken;
        this.value = identifierToken.value; // Değeri de sakla
        // Konum bilgisi de buradan alınabilir.
    }
}

// Literal İfade (Örnek: Sayı, String, Boolean)
class LiteralExpressionNode : ExpressionNode {
    const(Token) literalToken; // İlgili literal token (IntegerLiteralToken, StringLiteralToken vb.)
    // Literal değerin parse edilmiş hali de tutulabilir (int, double, string, bool)
     union { long intValue; double floatValue; string stringValue; bool boolValue; } parsedValue;

    this(const(Token) literalToken) {
        super(literalToken.kind); // Düğüm türü, literal token türü ile aynı olabilir
        this.literalToken = literalToken;
        this.value = literalToken.value; // Değeri de sakla
        // Burada literal değeri uygun türe parse etme mantığı eklenebilir.
    }
}

// İkili Operatör İfade (Örnek: a + b, x == y)
class BinaryOperatorExpressionNode : ExpressionNode {
    ASTNode* left;     // Sol işlenen (ifade düğümü)
    const(Token) operatorToken; // Operatör tokenı (+, -, == vb.)
    ASTNode* right;    // Sağ işlenen (ifade düğümü)

    this(ASTNode* left, const(Token) operatorToken, ASTNode* right) {
        super(SyntaxKind.BinaryOperatorExpressionNode); // Düğüm türünü belirle
        this.left = left;
        this.operatorToken = operatorToken;
        this.right = right;

        addChild(left);
        // Operatör tokenı genellikle alt düğüm olarak eklenmez, ama bilgi saklanır.
        addChild(right);
    }
}

// Kod Bloğu statement'ı ({ ... })
class BlockStatementNode : ASTNode {
    ASTNode[] statements; // Blok içindeki statement'lar

    this() {
        super(SyntaxKind.BlockStatementNode);
    }

    void addStatement(ASTNode statement) {
        if (statement) {
            this.statements ~= statement;
            addChild(statement); // Temel sınıftaki children listesine de ekle
        }
    }
}

// İfadeyi içeren Statement (örneğin, atama ifadesi statement olarak kullanılır: x = 5;)
class ExpressionStatementNode : ASTNode {
    ASTNode* expression; // Statement'ı oluşturan ifade

    this(ASTNode* expression) {
        super(SyntaxKind.ExpressionStatementNode);
        this.expression = expression;
        addChild(expression);
    }
}


// ... Dilin diğer yapıları için AST düğümleri tanımlanacaktır:
// IfStatementNode, SwitchStatementNode, MatchExpressionNode, CallExpressionNode,
// ParameterNode, TypeNode, ImportDeclarationNode vb.