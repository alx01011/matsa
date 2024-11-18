public class nr_gc_check {
    public static void main(String[] args) {
        Object obj = new Object();
        System.gc();
        synchronized (obj) {
            System.out.println("Hello, World!");
        }
    }   
}
