#include <iostream>
#include <vector>
#include <memory>
#include <stack>
#include <sstream>
#include <algorithm>
#include <string>

using namespace std;

// Базовий клас графічного об'єкта
class GraphicObject {
protected:
    int x, y;
public:
    GraphicObject(int x = 0, int y = 0) : x(x), y(y) {}
    virtual void draw(ostream& os, int indent = 0) const = 0;
    virtual bool containsPoint(int px, int py) const = 0;
    virtual shared_ptr<GraphicObject> clone() const = 0;
    virtual void move(int dx, int dy) { x += dx; y += dy; }
    virtual ~GraphicObject() = default;
    int getX() const { return x; }
    int getY() const { return y; }
};

// Коло
class Circle : public GraphicObject {
    int radius;
public:
    Circle(int x, int y, int r) : GraphicObject(x, y), radius(r) {}
    void draw(ostream& os, int indent = 0) const override {
        os << string(indent, '+') << "Circle (" << x << ", " << y << ") R=" << radius << "\n";
    }
    bool containsPoint(int px, int py) const override {
        int dx = px - x, dy = py - y;
        return dx*dx + dy*dy <= radius*radius;
    }
    shared_ptr<GraphicObject> clone() const override {
        return make_shared<Circle>(*this);
    }
};

// Прямокутник
class Rectangle : public GraphicObject {
    int width, height;
public:
    Rectangle(int x, int y, int w, int h) : GraphicObject(x, y), width(w), height(h) {}
    void draw(ostream& os, int indent = 0) const override {
        os << string(indent, '+') << "Rectangle (" << x << ", " << y << ") " << width << "*" << height << "\n";
    }
    bool containsPoint(int px, int py) const override {
        return (px >= x && px <= x + width && py >= y && py <= y + height);
    }
    shared_ptr<GraphicObject> clone() const override {
        return make_shared<Rectangle>(*this);
    }
};

// Група (Composite)
class Group : public GraphicObject, public enable_shared_from_this<Group> {
    vector<shared_ptr<GraphicObject>> children;
public:
    Group(int x = 0, int y = 0) : GraphicObject(x, y) {}

    void add(shared_ptr<GraphicObject> obj) {
        children.push_back(obj);
    }

    void draw(ostream& os, int indent = 0) const override {
        os << string(indent, '+') << "Group (" << x << ", " << y << ")\n";
        for (auto& child : children)
            child->draw(os, indent + 1);
    }

    bool containsPoint(int px, int py) const override {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if ((*it)->containsPoint(px - x, py - y))
                return true;
        }
        return false;
    }

    shared_ptr<GraphicObject> findDeepest(int px, int py) {
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if ((*it)->containsPoint(px - x, py - y)) {
                auto grp = dynamic_pointer_cast<Group>(*it);
                if (grp) return grp->findDeepest(px - x, py - y);
                else return *it;
            }
        }
        return shared_from_this();
    }

    shared_ptr<GraphicObject> clone() const override {
        auto newGroup = make_shared<Group>(x, y);
        for (auto& child : children)
            newGroup->add(child->clone());
        return newGroup;
    }

    vector<shared_ptr<GraphicObject>>& getChildren() { return children; }
};

// Команди (Command pattern)
class Command {
public:
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual ~Command() = default;
};

class AddCommand : public Command {
    vector<shared_ptr<GraphicObject>>& objects;
    shared_ptr<GraphicObject> obj;
public:
    AddCommand(vector<shared_ptr<GraphicObject>>& objs, shared_ptr<GraphicObject> obj)
        : objects(objs), obj(obj) {}
    void execute() override { objects.push_back(obj); }
    void undo() override { if(!objects.empty()) objects.pop_back(); }
};

// Фасад
class EditorFacade {
    vector<shared_ptr<GraphicObject>> objects;
    stack<shared_ptr<Command>> undoStack, redoStack;
public:
    void addObject(shared_ptr<GraphicObject> obj) {
        auto cmd = make_shared<AddCommand>(objects, obj);
        cmd->execute();
        undoStack.push(cmd);
        while (!redoStack.empty()) redoStack.pop();
    }

    void undo() {
        if (!undoStack.empty()) {
            auto cmd = undoStack.top(); undoStack.pop();
            cmd->undo();
            redoStack.push(cmd);
        } else {
            cout << "Немає дій для скасування.\n";
        }
    }

    void redo() {
        if (!redoStack.empty()) {
            auto cmd = redoStack.top(); redoStack.pop();
            cmd->execute();
            undoStack.push(cmd);
        } else {
            cout << "Немає дій для повторення.\n";
        }
    }

    void print() const {
        if (objects.empty()) {
            cout << "[Порожньо]\n";
            return;
        }
        for (auto& obj : objects)
            obj->draw(cout);
    }

    shared_ptr<GraphicObject> findElementAt(int x, int y) {
        for (auto it = objects.rbegin(); it != objects.rend(); ++it) {
            if ((*it)->containsPoint(x, y)) {
                auto grp = dynamic_pointer_cast<Group>(*it);
                if (grp) return grp->findDeepest(x, y);
                else return *it;
            }
        }
        return nullptr;
    }
};

// Функції для введення чисел із перевіркою
int readInt(const string& prompt) {
    int val;
    while (true) {
        cout << prompt;
        string line;
        getline(cin, line);
        stringstream ss(line);
        if (ss >> val) break;
        cout << "Некоректне число. Спробуйте ще.\n";
    }
    return val;
}

// Створення кола
shared_ptr<GraphicObject> createCircle() {
    int x = readInt("Введіть X центру кола: ");
    int y = readInt("Введіть Y центру кола: ");
    int r;
    while ((r = readInt("Введіть радіус кола (>0): ")) <= 0)
        cout << "Радіус має бути додатнім числом.\n";
    return make_shared<Circle>(x, y, r);
}

// Створення прямокутника
shared_ptr<GraphicObject> createRectangle() {
    int x = readInt("Введіть X лівого верхнього кута: ");
    int y = readInt("Введіть Y лівого верхнього кута: ");
    int w;
    while ((w = readInt("Введіть ширину (>0): ")) <= 0)
        cout << "Ширина має бути додатнім числом.\n";
    int h;
    while ((h = readInt("Введіть висоту (>0): ")) <= 0)
        cout << "Висота має бути додатнім числом.\n";
    return make_shared<Rectangle>(x, y, w, h);
}

// Рекурсивне створення групи
shared_ptr<Group> createGroup() {
    int x = readInt("Введіть X позицію групи: ");
    int y = readInt("Введіть Y позицію групи: ");
    auto group = make_shared<Group>(x, y);

    cout << "Додаємо об'єкти до групи. Введіть кількість об'єктів: ";
    int count = readInt("");
    for (int i = 0; i < count; ++i) {
        cout << "Виберіть тип об'єкта №" << (i + 1) << " для групи:\n";
        cout << "1. Circle\n2. Rectangle\n3. Group\nВаш вибір: ";
        int choice;
        cin >> choice;
        cin.ignore(); // зняти \n після числа

        shared_ptr<GraphicObject> obj = nullptr;
        if (choice == 1) obj = createCircle();
        else if (choice == 2) obj = createRectangle();
        else if (choice == 3) obj = createGroup();
        else {
            cout << "Невірний вибір, пропускаємо цей об'єкт.\n";
            continue;
        }
        group->add(obj);
    }
    return group;
}

// Головне меню
void menu(EditorFacade& editor) {
    while (true) {
        cout << "\n--- Меню редактора ---\n";
        cout << "1. Додати коло\n";
        cout << "2. Додати прямокутник\n";
        cout << "3. Додати групу об'єктів\n";
        cout << "4. Показати всі об'єкти\n";
        cout << "5. Undo\n";
        cout << "6. Redo\n";
        cout << "7. Знайти об'єкт за координатами\n";
        cout << "0. Вихід\n";
        cout << "Виберіть опцію: ";
        int choice;
        cin >> choice;
        cin.ignore();

        switch (choice) {
            case 1: {
                auto c = createCircle();
                editor.addObject(c);
                cout << "Коло додано.\n";
                break;
            }
            case 2: {
                auto r = createRectangle();
                editor.addObject(r);
                cout << "Прямокутник додано.\n";
                break;
            }
            case 3: {
                auto g = createGroup();
                editor.addObject(g);
                cout << "Група додана.\n";
                break;
            }
            case 4:
                cout << "Поточні об'єкти:\n";
                editor.print();
                break;
            case 5:
                editor.undo();
                break;
            case 6:
                editor.redo();
                break;
            case 7: {
                int x = readInt("Введіть X координату: ");
                int y = readInt("Введіть Y координату: ");
                auto found = editor.findElementAt(x, y);
                if (found) {
                    cout << "Знайдений об'єкт:\n";
                    found->draw(cout);
                } else {
                    cout << "Об'єктів на цій позиції не знайдено.\n";
                }
                break;
            }
            case 0:
                cout << "Вихід з програми...\n";
                return;
            default:
                cout << "Невірний вибір, спробуйте ще раз.\n";
        }
    }
}

int main() {
    EditorFacade editor;

    // Початкові об'єкти (за бажанням можна видалити)
    auto c1 = make_shared<Circle>(10, 10, 5);
    auto r1 = make_shared<Rectangle>(5, 7, 5, 6);

    auto group1 = make_shared<Group>(2, 2);
    group1->add(make_shared<Rectangle>(3, 4, 2, 3));
    group1->add(make_shared<Circle>(1, 5, 2));

    auto group2 = make_shared<Group>(4, 6);
    group2->add(make_shared<Circle>(0, 1, 3));
    group1->add(group2);

    editor.addObject(c1);
    editor.addObject(group1);
    editor.addObject(r1);

    cout << "Початкова структура:\n";
    editor.print();

    menu(editor);

    cout << "Натисніть Enter для завершення...";
    cin.get();
    return 0;
}
