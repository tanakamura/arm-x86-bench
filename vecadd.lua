function test()
	niter = 2
	nelem = 1024*1024

	src0 = {}
	src1 = {}

	for i=0, nelem
	do
		src0[i] = i*2
		src1[i] = i*3
	end

	for ii=0, niter
	do
		array = {}
		for ei=0, nelem
		do
			array[ei] = src0[ei] + src1[ei]
		end
	end
end

function test2()
	local niter = 32
	local nelem = 1024*1024
	local i, ii, ei

	local src0 = {}
	local src1 = {}

	for i=0, nelem
	do
		src0[i] = i*2
		src1[i] = i*3
	end

	for ii=0, niter
	do
		local array = {}
		for ei=0, nelem
		do
			array[ei] = src0[ei] + src1[ei]
		end
	end
end

test()