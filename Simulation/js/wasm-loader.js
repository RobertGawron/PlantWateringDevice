export let Module = null;

export async function loadModule() {
    Module = await createPlantWateringModule({
        print: (text) => console.log(text),
        printErr: (text) => console.error(text)
    });

    setTimeout(() => {
        Module._main();
    }, 100);
}