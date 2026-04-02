public class Threads extends Thread {
    static int counter = 0;
    static Threads lock;

    @Override
    public void run() {
        for (int i = 0; i < 100; i++) {
            synchronized (lock) {
                counter = counter + 1;
            }
        }
    }

    public static void main(String[] args) throws InterruptedException {
        lock = new Threads();
        Threads t1 = new Threads();
        Threads t2 = new Threads();
        t1.start();
        t2.start();
        t1.join();
        t2.join();
        System.out.println(counter);
    }
}
