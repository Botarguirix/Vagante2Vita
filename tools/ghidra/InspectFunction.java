// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.Function;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public class InspectFunction extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            throw new IllegalArgumentException("address output-file timeout-seconds");
        }
        Function function = currentProgram.getFunctionManager().getFunctionAt(toAddr(args[0]));
        if (function == null) throw new IllegalArgumentException("No function at " + args[0]);
        int timeout = Integer.parseInt(args[2]);
        DecompInterface decompiler = new DecompInterface();
        if (!decompiler.openProgram(currentProgram)) throw new IllegalStateException("Cannot open program");
        try {
            DecompileResults result = decompiler.decompileFunction(function, timeout, monitor);
            if (!result.decompileCompleted() || result.getDecompiledFunction() == null) {
                throw new IllegalStateException("Decompilation failed: " + result.getErrorMessage());
            }
            String output = "/* Ghidra focused decompilation: " + function.getName(true)
                    + " @ " + function.getEntryPoint() + " */\n"
                    + result.getDecompiledFunction().getC();
            Path path = Paths.get(args[1]);
            Files.createDirectories(path.toAbsolutePath().getParent());
            Files.writeString(path, output, StandardCharsets.UTF_8);
            println("Focused decompilation written: " + path);
        } finally {
            decompiler.dispose();
        }
    }
}
