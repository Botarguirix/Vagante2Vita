// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;

public class InspectAddresses extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("output-dir address...");
        Path root = Paths.get(args[0]);
        Files.createDirectories(root);
        DecompInterface decompiler = new DecompInterface();
        if (!decompiler.openProgram(currentProgram)) throw new IllegalStateException("Cannot open program");
        try {
            for (int i = 1; i < args.length; i++) {
                Address address = toAddr(args[i]);
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null) {
                    println("Missing function at " + address);
                    continue;
                }
                DecompileResults result = decompiler.decompileFunction(function, 180, monitor);
                if (!result.decompileCompleted() || result.getDecompiledFunction() == null) {
                    println("Failed " + function.getName(true) + ": " + result.getErrorMessage());
                    continue;
                }
                Path output = root.resolve(address.toString() + ".c");
                Files.writeString(output, "/* Focused Ghidra output, not verified source: "
                        + function.getName(true) + " @ " + address + " */\n"
                        + result.getDecompiledFunction().getC(), StandardCharsets.UTF_8);
                println("Wrote " + output);
            }
        } finally {
            decompiler.dispose();
        }
    }
}
