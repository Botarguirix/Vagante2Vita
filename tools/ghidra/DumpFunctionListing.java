// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class DumpFunctionListing extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 2) {
            throw new IllegalArgumentException("address output-file");
        }
        Address address = toAddr(args[0]);
        Function function = currentProgram.getFunctionManager().getFunctionAt(address);
        if (function == null) {
            function = currentProgram.getFunctionManager().getFunctionContaining(address);
        }
        if (function == null) {
            throw new IllegalArgumentException("No function at or containing " + address);
        }
        StringBuilder output = new StringBuilder();
        output.append(function.getName(true)).append(" @ ")
                .append(function.getEntryPoint()).append('\n');
        InstructionIterator instructions = currentProgram.getListing()
                .getInstructions(function.getBody(), true);
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            output.append(instruction.getAddress()).append("  ")
                    .append(instruction.toString()).append('\n');
        }
        Path destination = Paths.get(args[1]);
        Files.createDirectories(destination.toAbsolutePath().getParent());
        Files.writeString(destination, output, StandardCharsets.UTF_8);
        println("Function listing written: " + destination);
    }
}
