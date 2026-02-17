#include <QTest>

class TestA: public QObject
{
    Q_OBJECT

private slots:
    void testCase() { QVERIFY(true); }
};

#include <test_main.moc>

int main()
{
    TestA test_a;
    return QTest::qExec(&test_a);
}
