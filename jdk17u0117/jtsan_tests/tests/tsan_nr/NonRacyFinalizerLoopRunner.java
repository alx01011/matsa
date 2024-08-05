public class NonRacyFinalizerLoopRunner {
    static class Foo {
        int x;
    
        public Foo() {
          x = 5;
        }
    
        @Override
        public void finalize() {
          x = 7;
        }
    }

    public static void main(String[] args) throws InterruptedException {
        for (int i = 0; i < 100; ++i) {
            Foo foo = new Foo();
            foo.x = 5;
            System.gc();
          }
    }
}
