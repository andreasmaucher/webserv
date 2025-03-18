import "dotenv/config";
import { defineConfig } from "drizzle-kit";
export default defineConfig({
  out: "./drizzle",
  schema: "./db/schema/*.ts",
  dialect: "postgresql",
  dbCredentials: {
    url: process.env.DATABASE_URL!,
  },
});

------WebKitFormBoundaryoRrzx4jIMWpjKYT5--
Content-Type: application/json

{
  "status": "success",
  "files": [
    {
      "name": "drizzle.config.ts",
      "size": 237,
      "path": "/uploads/drizzle.config.ts"
    }
  ]
}
