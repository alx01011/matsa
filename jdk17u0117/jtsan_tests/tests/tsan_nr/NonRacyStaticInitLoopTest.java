public class NonRacyStaticInitLoopTest {
    private static int x;

    static {
      x = 5;
    }
  
    public static void main(String[] args) throws InterruptedException {
      Thread t =
          new Thread(
              () -> {
                x = 2;
              });
      t.start();
      t.join();
    }
}
