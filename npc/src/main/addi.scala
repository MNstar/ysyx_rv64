package winner

import chisel3._

class PC extends Module {
    val io = IO(new Bundle{
        val in = Input(UInt(32.W))
        val out = Output(UInt(32.W))
    })
    io.out := io.in + 4.U
}

class Alu extends Module {
    val io = IO(new Bundle{
        val 
    })
}
