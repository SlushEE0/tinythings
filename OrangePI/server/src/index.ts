import express from "express";

const { PORT } = process.env;

const app = express();

app.get("*", (req, res) => {
  switch (req.path) {
    case "/api/test":
      res.json("Docker esta muy facil");
      break;

    case "/":
      res.send(`<h1>Hellow</h1>`);
      break;

    default:
      res.status(404).send(`Endpoint '${req.path}' not found`);
  }
});

app.listen(PORT || 8080, () => {
  console.log("Hello via Bun!");
});
