#include <QApplication>


// Initializer.
__attribute__((constructor))
  static void initializer(void) {  
    printf("[%s] initializer()\n", __FILE__);
  }

// Finalizer.
__attribute__((destructor))
  static void finalizer(void) {                               // 3
    printf("[%s] finalizer()\n", __FILE__);
  }

int foo() {
  return 0;
}
